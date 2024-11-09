#include "test/MXLight.h"
#include "test/RendererMX.h"

#include <MaterialXFormat/Util.h>
#include <MaterialXRender/TinyObjLoader.h>
#include <MaterialXRender/Harmonics.h>

#include <iostream>

namespace Hazel {

    namespace
    {
        const int IRRADIANCE_MAP_WIDTH = 256;
        const int IRRADIANCE_MAP_HEIGHT = 128;

        const std::string DIR_LIGHT_NODE_CATEGORY = "directional_light";
        const std::string IRRADIANCE_MAP_FOLDER = "irradiance";

        const float ENV_MAP_SPLIT_RADIANCE = 16.0f;
        const float MAX_ENV_TEXEL_RADIANCE = 100000.0f;
        const float IDEAL_ENV_MAP_RADIANCE = 6.0f;

    } // anonymous namespace

    MXLight::MXLight() :
        _envRadianceFilename("resources/Lights/san_giuseppe_bridge_split.hdr"),
        _lightRotation(0.0f),
        _normalizeEnvironment(false),
        _splitDirectLight(false),
        _generateReferenceIrradiance(false),
        _saveGeneratedLights(false),
        _lightHandler(mx::LightHandler::create()),
        _quadMesh(mx::GeometryHandler::createQuadMesh())
    {
    }

    void MXLight::createEnvGeometryHandler()
    {
        // Create environment geometry handler.
        mx::TinyObjLoaderPtr objLoader = mx::TinyObjLoader::create();
        _envGeometryHandler = mx::GeometryHandler::create();
        _envGeometryHandler->addLoader(objLoader);
        mx::FilePath envSphere("resources/Geometry/sphere.obj");
        _envGeometryHandler->loadGeometry(RendererMX::s_Data->_searchPath.find(envSphere));
    }

    void MXLight::loadEnvironmentLight()
    {
        // Load the requested radiance map.
        mx::ImagePtr envRadianceMap = RendererMX::s_Data->_imageHandler->acquireImage(_envRadianceFilename);
        if (!envRadianceMap)
        {
            //new ng::MessageDialog(this, ng::MessageDialog::Type::Warning, "Failed to load environment light");
            return;
        }

        // If requested, normalize the environment upon loading.
        if (_normalizeEnvironment)
        {
            envRadianceMap = mx::normalizeEnvironment(envRadianceMap, IDEAL_ENV_MAP_RADIANCE, MAX_ENV_TEXEL_RADIANCE);
            if (_saveGeneratedLights)
            {
                RendererMX::s_Data->_imageHandler->saveImage("NormalizedRadiance.hdr", envRadianceMap);
            }
        }

        // If requested, split the environment into indirect and direct components.
        if (_splitDirectLight)
        {
            splitDirectLight(envRadianceMap, envRadianceMap, _lightRigDoc);
            if (_saveGeneratedLights)
            {
                RendererMX::s_Data->_imageHandler->saveImage("IndirectRadiance.hdr", envRadianceMap);
                mx::writeToXmlFile(_lightRigDoc, "DirectLightRig.mtlx");
            }
        }

        // Look for an irradiance map using an expected filename convention.
        mx::ImagePtr envIrradianceMap;
        if (!_normalizeEnvironment && !_splitDirectLight)
        {
            mx::FilePath envIrradiancePath = _envRadianceFilename.getParentPath() / IRRADIANCE_MAP_FOLDER / _envRadianceFilename.getBaseName();
            envIrradianceMap = RendererMX::s_Data->_imageHandler->acquireImage(envIrradiancePath);
        }

        // If not found, then generate an irradiance map via spherical harmonics.
        if (!envIrradianceMap || envIrradianceMap->getWidth() == 1)
        {
            if (_generateReferenceIrradiance)
            {
                envIrradianceMap = mx::renderReferenceIrradiance(envRadianceMap, IRRADIANCE_MAP_WIDTH, IRRADIANCE_MAP_HEIGHT);
                if (_saveGeneratedLights)
                {
                    RendererMX::s_Data->_imageHandler->saveImage("ReferenceIrradiance.hdr", envIrradianceMap);
                }
            }
            else
            {
                mx::Sh3ColorCoeffs shIrradiance = mx::projectEnvironment(envRadianceMap, true);
                envIrradianceMap = mx::renderEnvironment(shIrradiance, IRRADIANCE_MAP_WIDTH, IRRADIANCE_MAP_HEIGHT);
                if (_saveGeneratedLights)
                {
                    RendererMX::s_Data->_imageHandler->saveImage("SphericalHarmonicIrradiance.hdr", envIrradianceMap);
                }
            }
        }

        // Release any existing environment maps and store the new ones.
        RendererMX::s_Data->_imageHandler->releaseRenderResources(_lightHandler->getEnvRadianceMap());
        RendererMX::s_Data->_imageHandler->releaseRenderResources(_lightHandler->getEnvPrefilteredMap());
        RendererMX::s_Data->_imageHandler->releaseRenderResources(_lightHandler->getEnvIrradianceMap());

        _lightHandler->setEnvRadianceMap(envRadianceMap);
        _lightHandler->setEnvIrradianceMap(envIrradianceMap);
        _lightHandler->setEnvPrefilteredMap(nullptr);

        // Look for a light rig using an expected filename convention.
        if (!_splitDirectLight)
        {
            _lightRigFilename = _envRadianceFilename;
            _lightRigFilename.removeExtension();
            _lightRigFilename.addExtension(mx::MTLX_EXTENSION);
            _lightRigFilename = RendererMX::s_Data->_searchPath.find(_lightRigFilename);
            if (_lightRigFilename.exists())
            {
                _lightRigDoc = mx::createDocument();
                mx::readFromXmlFile(_lightRigDoc, _lightRigFilename, RendererMX::s_Data->_searchPath);
            }
            else
            {
                _lightRigDoc = nullptr;
            }
        }

        // Invalidate the existing environment material, if any.
        _envMaterial = nullptr;
    }

    void MXLight::splitDirectLight(mx::ImagePtr envRadianceMap, mx::ImagePtr& indirectMap, mx::DocumentPtr& dirLightDoc)
    {
        mx::Vector3 lightDir;
        mx::Color3 lightColor;
        mx::ImagePair imagePair = envRadianceMap->splitByLuminance(ENV_MAP_SPLIT_RADIANCE);

        mx::computeDominantLight(imagePair.second, lightDir, lightColor);
        float lightIntensity = std::max(std::max(lightColor[0], lightColor[1]), lightColor[2]);
        if (lightIntensity)
        {
            lightColor /= lightIntensity;
        }

        dirLightDoc = mx::createDocument();
        mx::NodePtr dirLightNode = dirLightDoc->addNode(DIR_LIGHT_NODE_CATEGORY, "dir_light", mx::LIGHT_SHADER_TYPE_STRING);
        dirLightNode->setInputValue("direction", lightDir);
        dirLightNode->setInputValue("color", lightColor);
        dirLightNode->setInputValue("intensity", lightIntensity);
        indirectMap = imagePair.first;
    }

    mx::MaterialPtr MXLight::getEnvironmentMaterial()
    {
        if (!_envMaterial)
        {
            mx::FilePath envFilename = RendererMX::s_Data->_searchPath.find(mx::FilePath("resources/Lights/environment_map.mtlx"));
            try
            {
                _envMaterial = RendererMX::s_Data->_renderPipeline->createMaterial();
                _envMaterial->generateEnvironmentShader(RendererMX::s_Data->_genContext, envFilename, RendererMX::s_Data->_stdLib, _envRadianceFilename);
            }
            catch (std::exception& e)
            {
                std::cerr << "Failed to generate environment shader: " << e.what() << std::endl;
                _envMaterial = nullptr;
            }
        }

        return _envMaterial;
    }

}