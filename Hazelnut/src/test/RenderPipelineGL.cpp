#include "test/RenderPipelineGL.h"
#include "test/Viewer.h"

#include <MaterialXRenderGlsl/GLTextureHandler.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXRender/ShaderRenderer.h>
#include <MaterialXRender/StbImageLoader.h>
#include <MaterialXRenderGlsl/GLFramebuffer.h>
#include <MaterialXRenderGlsl/GlslMaterial.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cassert>

namespace Hazel {

    namespace
    {
        const float PI = std::acos(-1.0f);
    }

    GLRenderPipeline::GLRenderPipeline(Viewer* viewerPtr) :
        RenderPipeline(viewerPtr)
    {
    }

    void GLRenderPipeline::initialize(void*, void*)
    {
    }

    mx::ImageHandlerPtr GLRenderPipeline::createImageHandler()
    {
        return mx::GLTextureHandler::create(mx::StbImageLoader::create());
    }

    mx::MaterialPtr GLRenderPipeline::createMaterial()
    {
        return mx::GlslMaterial::create();
    }

    void GLRenderPipeline::initFramebuffer(int, int, void*)
    {
    }

    void GLRenderPipeline::resizeFramebuffer(int, int, void*)
    {
    }

    void GLRenderPipeline::updatePrefilteredMap()
    {
        auto& genContext = _viewer->_genContext;
        auto& lightHandler = _viewer->_light->getLightHandler();
        auto& imageHandler = _viewer->_imageHandler;

        if (lightHandler->getEnvPrefilteredMap())
        {
            return;
        }

        // Create the prefilter shader.
        mx::GlslMaterialPtr material = nullptr;
        try
        {
            mx::ShaderPtr hwShader = mx::createEnvPrefilterShader(genContext, _viewer->_stdLib, "__ENV_PREFILTER__");
            material = mx::GlslMaterial::create();
            material->generateShader(hwShader);
        }
        catch (std::exception& e)
        {
            //new ng::MessageDialog(_viewer, ng::MessageDialog::Type::Warning, "Failed to generate prefilter shader", e.what());
        }

        mx::ImagePtr srcTex = lightHandler->getEnvRadianceMap();

        int w = srcTex->getWidth();
        int h = srcTex->getHeight();
        int numMips = srcTex->getMaxMipCount();

        // Create texture to hold the prefiltered environment.
        mx::GLTextureHandlerPtr glImageHandler = std::dynamic_pointer_cast<mx::GLTextureHandler>(imageHandler);
        mx::ImagePtr outTex = mx::Image::create(w, h, 3, mx::Image::BaseType::HALF);
        glImageHandler->createRenderResources(outTex, true, true);

        mx::GlslProgramPtr program = material->getProgram();

        try
        {
            int i = 0;
            while (w > 0 && h > 0)
            {
                // Create framebuffer
                unsigned int framebuffer;
                glGenFramebuffers(1, &framebuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex->getResourceId(), i);
                glViewport(0, 0, w, h);
                material->bindShader();

                // Bind the source texture
                mx::ImageSamplingProperties samplingProperties;
                samplingProperties.uaddressMode = mx::ImageSamplingProperties::AddressMode::PERIODIC;
                samplingProperties.vaddressMode = mx::ImageSamplingProperties::AddressMode::CLAMP;
                samplingProperties.filterType = mx::ImageSamplingProperties::FilterType::LINEAR;
                imageHandler->bindImage(srcTex, samplingProperties);
                int textureLocation = glImageHandler->getBoundTextureLocation(srcTex->getResourceId());
                assert(textureLocation >= 0);
                material->getProgram()->bindUniform(mx::HW::ENV_RADIANCE, mx::Value::createValue(textureLocation));
                // Bind other uniforms
                program->bindUniform(mx::HW::ENV_PREFILTER_MIP, mx::Value::createValue(i));
                const mx::Matrix44 yRotationPI = mx::Matrix44::createScale(mx::Vector3(-1, 1, -1));
                program->bindUniform(mx::HW::ENV_MATRIX, mx::Value::createValue(yRotationPI));
                program->bindUniform(mx::HW::ENV_RADIANCE_MIPS, mx::Value::createValue<int>(numMips));

                _viewer->renderScreenSpaceQuad(material);

                glDeleteFramebuffers(1, &framebuffer);

                w /= 2;
                h /= 2;
                i++;
            }
        }
        catch (mx::ExceptionRenderError& e)
        {
            for (const std::string& error : e.errorLog())
            {
                std::cerr << error << std::endl;
            }
            //new ng::MessageDialog(_viewer, ng::MessageDialog::Type::Warning, "Failed to render prefiltered environment", e.what());
        }
        catch (std::exception& e)
        {
            //new ng::MessageDialog(_viewer, ng::MessageDialog::Type::Warning, "Failed to render prefiltered environment", e.what());
        }

        // Clean up.
        glViewport(0, 0, _viewer->m_fbsize[0], _viewer->m_fbsize[1]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        lightHandler->setEnvPrefilteredMap(outTex);
    }

    mx::ImagePtr GLRenderPipeline::getShadowMap(int shadowMapSize)
    {
        auto& genContext = _viewer->_genContext;
        auto& imageHandler = _viewer->_imageHandler;
        auto& shadowCamera = _viewer->_camera->getShadowCamera();
        auto& stdLib = _viewer->_stdLib;
        auto& geometryHandler = _viewer->_mesh->getGeometryHandler();

        if (!_viewer->_shadowMap)
        {
            // Generate shaders for shadow rendering.
            if (!_viewer->_shadowMaterial)
            {
                try
                {
                    mx::ShaderPtr hwShader = mx::createDepthShader(genContext, stdLib, "__SHADOW_SHADER__");
                    _viewer->_shadowMaterial = mx::GlslMaterial::create();
                    _viewer->_shadowMaterial->generateShader(hwShader);
                }
                catch (std::exception& e)
                {
                    std::cerr << "Failed to generate shadow shader: " << e.what() << std::endl;
                    _viewer->_shadowMaterial = nullptr;
                }
            }
            if (!_viewer->_shadowBlurMaterial)
            {
                try
                {
                    mx::ShaderPtr hwShader = mx::createBlurShader(genContext, stdLib, "__SHADOW_BLUR_SHADER__", "gaussian", 1.0f);
                    _viewer->_shadowBlurMaterial = mx::GlslMaterial::create();
                    _viewer->_shadowBlurMaterial->generateShader(hwShader);
                }
                catch (std::exception& e)
                {
                    std::cerr << "Failed to generate shadow blur shader: " << e.what() << std::endl;
                    _viewer->_shadowBlurMaterial = nullptr;
                }
            }

            if (_viewer->_shadowMaterial && _viewer->_shadowBlurMaterial)
            {
                // Create framebuffer.
                mx::GLFramebufferPtr framebuffer = mx::GLFramebuffer::create(shadowMapSize, shadowMapSize, 2, mx::Image::BaseType::FLOAT);
                framebuffer->bind();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

                // Render shadow geometry.
                _viewer->_shadowMaterial->bindShader();
                for (auto mesh : geometryHandler->getMeshes())
                {
                    _viewer->_shadowMaterial->bindMesh(mesh);
                    _viewer->_shadowMaterial->bindViewInformation(shadowCamera);
                    for (size_t i = 0; i < mesh->getPartitionCount(); i++)
                    {
                        mx::MeshPartitionPtr geom = mesh->getPartition(i);
                        _viewer->_shadowMaterial->drawPartition(geom);
                    }
                }
                _viewer->_shadowMap = framebuffer->getColorImage();

                // Apply Gaussian blurring.
                mx::ImageSamplingProperties blurSamplingProperties;
                blurSamplingProperties.uaddressMode = mx::ImageSamplingProperties::AddressMode::CLAMP;
                blurSamplingProperties.vaddressMode = mx::ImageSamplingProperties::AddressMode::CLAMP;
                blurSamplingProperties.filterType = mx::ImageSamplingProperties::FilterType::CLOSEST;
                for (unsigned int i = 0; i < _viewer->_shadowSoftness; i++)
                {
                    framebuffer->bind();
                    _viewer->_shadowBlurMaterial->bindShader();
                    if (imageHandler->bindImage(_viewer->_shadowMap, blurSamplingProperties))
                    {
                        mx::GLTextureHandlerPtr textureHandler = std::static_pointer_cast<mx::GLTextureHandler>(imageHandler);
                        int textureLocation = textureHandler->getBoundTextureLocation(_viewer->_shadowMap->getResourceId());
                        if (textureLocation >= 0)
                        {
                            std::static_pointer_cast<mx::GlslMaterial>(_viewer->_shadowBlurMaterial)
                                ->getProgram()->bindUniform("image_file", mx::Value::createValue(textureLocation));
                        }
                    }
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
                    _viewer->renderScreenSpaceQuad(_viewer->_shadowBlurMaterial);
                    imageHandler->releaseRenderResources(_viewer->_shadowMap);
                    _viewer->_shadowMap = framebuffer->getColorImage();
                }

                // Restore state for scene rendering.
                glViewport(0, 0, _viewer->m_fbsize[0], _viewer->m_fbsize[1]);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                glDrawBuffer(GL_BACK);
            }
        }

        return _viewer->_shadowMap;
    }


    void GLRenderPipeline::renderFrame(void*, int shadowMapSize, const char* dirLightNodeCat)
    {
        auto& genContext = _viewer->_genContext;
        auto& lightHandler = _viewer->_light->getLightHandler();
        auto& imageHandler = _viewer->_imageHandler;
        auto& viewCamera = _viewer->_camera->getViewCamera();
        auto& envCamera = _viewer->_camera->getEnvCamera();
        auto& shadowCamera = _viewer->_camera->getShadowCamera();
        float lightRotation = _viewer->_light->getLightRotation();
        auto& searchPath = _viewer->_searchPath;
        auto& geometryHandler = _viewer->_mesh->getGeometryHandler();

        // Update prefiltered environment.
        if (lightHandler->getUsePrefilteredMap() && !_viewer->_mesh->getMaterialAssignments().empty())
        {
            updatePrefilteredMap();
        }

        // Initialize OpenGL state
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_CULL_FACE);
        glDisable(GL_FRAMEBUFFER_SRGB);

        // Update lighting state.
        lightHandler->setLightTransform(mx::Matrix44::createRotationY(lightRotation / 180.0f * PI));

        // Update shadow state.
        mx::ShadowState shadowState;
        shadowState.ambientOcclusionGain = _viewer->_ambientOcclusionGain;
        mx::NodePtr dirLight = lightHandler->getFirstLightOfCategory(dirLightNodeCat);
        if (genContext.getOptions().hwShadowMap && dirLight)
        {
            mx::ImagePtr shadowMap = getShadowMap(shadowMapSize);
            if (shadowMap)
            {
                shadowState.shadowMap = shadowMap;
                shadowState.shadowMatrix = viewCamera->getWorldMatrix().getInverse() *
                    shadowCamera->getWorldViewProjMatrix();
            }
            else
            {
                genContext.getOptions().hwShadowMap = false;
            }
        }

        glEnable(GL_FRAMEBUFFER_SRGB);

        // Environment background
        if (_viewer->_drawEnvironment)
        {
            mx::MaterialPtr envMaterial = _viewer->_light->getEnvironmentMaterial();
            if (envMaterial)
            {
                const mx::MeshList& meshes = _viewer->_light->getEnvGeometryHandler()->getMeshes();
                mx::MeshPartitionPtr envPart = !meshes.empty() ? meshes[0]->getPartition(0) : nullptr;
                if (envPart)
                {
                    // Apply rotation to the environment shader.
                    float longitudeOffset = (lightRotation / 360.0f) + 0.5f;
                    envMaterial->modifyUniform("longitude/in2", mx::Value::createValue(longitudeOffset));

                    // Apply light intensity to the environment shader.
                    envMaterial->modifyUniform("envImageAdjusted/in2", mx::Value::createValue(lightHandler->getEnvLightIntensity()));

                    // Render the environment mesh.
                    glDepthMask(GL_FALSE);
                    envMaterial->bindShader();
                    envMaterial->bindMesh(meshes[0]);
                    envMaterial->bindViewInformation(envCamera);
                    envMaterial->bindImages(imageHandler, searchPath, false);
                    envMaterial->drawPartition(envPart);
                    glDepthMask(GL_TRUE);
                }
            }
            else
            {
                _viewer->_drawEnvironment = false;
            }
        }

        // Enable backface culling if requested.
        if (!_viewer->_renderDoubleSided)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }

        // Opaque pass
        for (const auto& assignment : _viewer->_mesh->getMaterialAssignments())
        {
            mx::MeshPartitionPtr geom = assignment.first;
            mx::GlslMaterialPtr material = std::dynamic_pointer_cast<mx::GlslMaterial>(assignment.second);
            shadowState.ambientOcclusionMap = _viewer->_mesh->getAmbientOcclusionImage(material);
            if (!material)
            {
                continue;
            }

            material->bindShader();
            material->bindMesh(geometryHandler->findParentMesh(geom));
            if (material->getProgram()->hasUniform(mx::HW::ALPHA_THRESHOLD))
            {
                material->getProgram()->bindUniform(mx::HW::ALPHA_THRESHOLD, mx::Value::createValue(0.99f));
            }
            material->bindViewInformation(viewCamera);
            material->bindLighting(lightHandler, imageHandler, shadowState);
            material->bindImages(imageHandler, searchPath);
            material->drawPartition(geom);
            material->unbindImages(imageHandler);
        }

        // Transparent pass
        if (_viewer->_renderTransparency)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            for (const auto& assignment : _viewer->_mesh->getMaterialAssignments())
            {
                mx::MeshPartitionPtr geom = assignment.first;
                mx::GlslMaterialPtr material = std::dynamic_pointer_cast<mx::GlslMaterial>(assignment.second);
                shadowState.ambientOcclusionMap = _viewer->_mesh->getAmbientOcclusionImage(material);
                if (!material || !material->hasTransparency())
                {
                    continue;
                }

                material->bindShader();
                material->bindMesh(geometryHandler->findParentMesh(geom));
                if (material->getProgram()->hasUniform(mx::HW::ALPHA_THRESHOLD))
                {
                    material->getProgram()->bindUniform(mx::HW::ALPHA_THRESHOLD, mx::Value::createValue(0.001f));
                }
                material->bindViewInformation(viewCamera);
                material->bindLighting(lightHandler, imageHandler, shadowState);
                material->bindImages(imageHandler, searchPath);
                material->drawPartition(geom);
                material->unbindImages(imageHandler);
            }
            glDisable(GL_BLEND);
        }

        if (!_viewer->_renderDoubleSided)
        {
            glDisable(GL_CULL_FACE);
        }
        glDisable(GL_FRAMEBUFFER_SRGB);

        // Wireframe pass
        if (_viewer->_outlineSelection)
        {
            mx::MaterialPtr wireMaterial = _viewer->_mesh->getWireframeMaterial();
            if (wireMaterial)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(3.0f);
                wireMaterial->bindShader();
                wireMaterial->bindMesh(geometryHandler->findParentMesh(_viewer->_mesh->getSelectedGeometry()));
                wireMaterial->bindViewInformation(viewCamera);
                wireMaterial->drawPartition(_viewer->_mesh->getSelectedGeometry());
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }
            else
            {
                _viewer->_outlineSelection = false;
            }
        }
    }


}