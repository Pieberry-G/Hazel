#include "test/RendererMX.h"

#include "test/MXUtils.h"

#include "Hazel/Renderer/RenderCommand.h"
#include "Hazel/Renderer/Texture.h"

#include <MaterialXRenderGlsl/GLUtil.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXFormat/Util.h>

namespace Hazel {

    namespace
    {
        const int SHADOW_MAP_SIZE = 2048;
        const std::string DIR_LIGHT_NODE_CATEGORY = "directional_light";

    } // anonymous namespace

    RendererMXData* RendererMX::s_Data = nullptr;

    void RendererMX::Init()
    {
        s_Data = new RendererMXData(mx::FileSearchPath("../../Hazel/deps/MaterialX"),
            { "libraries" },
            mx::DEFAULT_SCREEN_COLOR_SRGB);

        // Initialize the standard libraries and color/unit management.
        loadStandardLibraries();

        // Initialize image handler.
        s_Data->_imageHandler = s_Data->_renderPipeline->createImageHandler();
        s_Data->_imageHandler->setSearchPath(s_Data->_searchPath);

        //s_Data->_mesh->createGeometryHandler();

        //s_Data->_renderPipeline->initFramebuffer(1920.0f, 1080.0f, nullptr);

        s_Data->_light->createEnvGeometryHandler();

        // Initialize environment light.
        s_Data->_light->loadEnvironmentLight();

        //// Load the requested material document.
        //s_Data->_mesh->loadDocument(s_Data->_stdLib);


        // Gamma Correction
        s_Data->GammaShader = Shader::Create("../../assets/shaders/GammaCorrection.glsl");
        s_Data->GammaShader->Bind();
        s_Data->GammaShader->SetInt("u_Texture", 0);
        s_Data->GammaVertexArray = VertexArray::Create();

        Ref<VertexBuffer> gammaVB = VertexBuffer::Create(4 * sizeof(QuadVertex));
        gammaVB->SetLayout({
            { ShaderDataType::Float3, "a_Position"	},
            { ShaderDataType::Float2, "a_TexCoord"	},
        });
        s_Data->GammaVertexArray->AddVertexBuffer(gammaVB);

        QuadVertex* quadVertices = new QuadVertex[4];
        quadVertices[0] = { {-1.0f, -1.0f, 0.0f}, { 0.0f, 0.0f} };
        quadVertices[1] = { { 1.0f, -1.0f, 0.0f}, { 1.0f, 0.0f} };
        quadVertices[2] = { { 1.0f,  1.0f, 0.0f}, { 1.0f, 1.0f} };
        quadVertices[3] = { {-1.0f,  1.0f, 0.0f}, { 0.0f, 1.0f} };

        gammaVB->SetData(quadVertices, 4 * sizeof(QuadVertex));
        delete[] quadVertices;

        uint32_t* quadIndices = new uint32_t[6];
        quadIndices[0] = 0;
        quadIndices[1] = 1;
        quadIndices[2] = 2;
        quadIndices[3] = 2;
        quadIndices[4] = 3;
        quadIndices[5] = 0;

        Ref<IndexBuffer> gammaIB = IndexBuffer::Create(quadIndices, 6);
        s_Data->GammaVertexArray->SetIndexBuffer(gammaIB);
        delete[] quadIndices;

        // Lines
        s_Data->LineShader = Shader::Create("../../assets/shaders/DrawLine.glsl");
        s_Data->LineVertexBufferBase = new LineVertex[100000];

        // Pick Buffer
        s_Data->PickShader = Shader::Create("../../assets/shaders/PickBuffer.glsl");
    }

    void RendererMX::BeginScene(const EditorCamera& camera)
    {
        glm::vec3 camPos = camera.GetPosition();
        glm::mat4 viewProj = camera.GetViewProjection();
        glm::mat4 viewMatrix = camera.GetViewMatrix();
        glm::mat4 projection = camera.GetProjection();

        mx::CameraPtr viewCamera = s_Data->_camera->GetViewCamera();
        viewCamera->setViewMatrix(MXUtils::GlmMat4ToMaterialXMat4(viewMatrix));
        viewCamera->setProjectionMatrix(MXUtils::GlmMat4ToMaterialXMat4(projection));

        s_Data->LineShader->Bind();
        s_Data->LineShader->SetMat4("u_ViewProjection", viewProj);
        s_Data->LineShader->SetFloat3("u_CamPos", camPos);
        s_Data->LineVertexCount = 0;
        s_Data->LineVertexBufferPtr = s_Data->LineVertexBufferBase;
    }

    void RendererMX::EndScene()
    {
        if (s_Data->LineVertexCount)
        {
            // VAO
            s_Data->LineVertexArray = VertexArray::Create();
            // VBO
            uint32_t dataSize = (uint8_t*)s_Data->LineVertexBufferPtr - (uint8_t*)s_Data->LineVertexBufferBase;
            s_Data->LineVertexBuffer = VertexBuffer::Create(s_Data->LineVertexBufferBase, dataSize);
            s_Data->LineVertexBuffer->SetLayout({
                { ShaderDataType::Float3, "a_Position"	},
                { ShaderDataType::Float4, "a_Color"		},
            });
            s_Data->LineVertexArray->AddVertexBuffer(s_Data->LineVertexBuffer);

            s_Data->LineShader->Bind();
            RenderCommand::SetLineWidth(s_Data->LineWidth);
            RenderCommand::DrawLines(s_Data->LineVertexArray, s_Data->LineVertexCount);
        }
    }

    void RendererMX::initContext(mx::GenContext& context)
    {
        // Initialize search path.
        context.registerSourceCodeSearchPath(s_Data->_searchPath);

        // Initialize color management.
        mx::DefaultColorManagementSystemPtr cms = mx::DefaultColorManagementSystem::create(context.getShaderGenerator().getTarget());
        cms->loadLibrary(s_Data->_stdLib);
        context.getShaderGenerator().setColorManagementSystem(cms);

        // Initialize unit management.
        mx::UnitSystemPtr unitSystem = mx::UnitSystem::create(context.getShaderGenerator().getTarget());
        unitSystem->loadLibrary(s_Data->_stdLib);
        unitSystem->setUnitConverterRegistry(s_Data->_unitRegistry);
        context.getShaderGenerator().setUnitSystem(unitSystem);
        context.getOptions().targetDistanceUnit = "meter";
    }

    void RendererMX::loadStandardLibraries()
    {
        // Initialize the standard library.
        try
        {
            s_Data->_stdLib = mx::createDocument();
            s_Data->_xincludeFiles = mx::loadLibraries(s_Data->_libraryFolders, s_Data->_searchPath, s_Data->_stdLib);
            if (s_Data->_xincludeFiles.empty())
            {
                std::cerr << "Could not find standard data libraries on the given search path: " << s_Data->_searchPath.asString() << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to load standard data libraries: " << e.what() << std::endl;
            return;
        }

        // Initialize unit management.
        mx::UnitTypeDefPtr distanceTypeDef = s_Data->_stdLib->getUnitTypeDef("distance");
        s_Data->_distanceUnitConverter = mx::LinearUnitConverter::create(distanceTypeDef);
        s_Data->_unitRegistry->addUnitConverter(distanceTypeDef, s_Data->_distanceUnitConverter);
        mx::UnitTypeDefPtr angleTypeDef = s_Data->_stdLib->getUnitTypeDef("angle");
        mx::LinearUnitConverterPtr angleConverter = mx::LinearUnitConverter::create(angleTypeDef);
        s_Data->_unitRegistry->addUnitConverter(angleTypeDef, angleConverter);

        // Create the list of supported distance units.
        auto unitScales = s_Data->_distanceUnitConverter->getUnitScale();
        s_Data->_distanceUnitOptions.resize(unitScales.size());
        for (auto unitScale : unitScales)
        {
            int location = s_Data->_distanceUnitConverter->getUnitAsInteger(unitScale.first);
            s_Data->_distanceUnitOptions[location] = unitScale.first;
        }

        // Initialize the generator contexts.
        initContext(s_Data->_genContext);
    }

    void RendererMX::renderScreenSpaceQuad(mx::MaterialPtr material)
    {
        material->bindMesh(s_Data->_light->getQuadMesh());
        material->drawPartition(s_Data->_light->getQuadMesh()->getPartition(0));
    }

    void RendererMX::DrawMesh(MXMesh& mesh, TransformComponent& tc)
    {
        s_Data->_camera->UpdateCameras(mesh, tc, s_Data->_light);

        mx::checkGlErrors("before viewer render");

        // Render the current frame.
        try
        {
            s_Data->_renderPipeline->renderFrame(mesh, s_Data->_colorTexture,
                SHADOW_MAP_SIZE,
                DIR_LIGHT_NODE_CATEGORY.c_str());
        }
        catch (std::exception& e)
        {
            //new ng::MessageDialog(this, ng::MessageDialog::Type::Warning,
            //    "Failed to render frame: ", e.what());
            //_mesh->_materialAssignments.clear();
            glDisable(GL_FRAMEBUFFER_SRGB);
        }

        mx::checkGlErrors("after viewer render");
    }

    void RendererMX::DrawLines(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
    {
        s_Data->LineVertexBufferPtr->Position = p0;
        s_Data->LineVertexBufferPtr->Color = color;
        s_Data->LineVertexBufferPtr++;
        s_Data->LineVertexBufferPtr->Position = p1;
        s_Data->LineVertexBufferPtr->Color = color;
        s_Data->LineVertexBufferPtr++;

        s_Data->LineVertexCount += 2;
    }

    void RendererMX::DrawGroundPlane(int rows, int cols, float spacing)
    {
        glm::vec4 color(0.8f);
        for (int i = 0; i <= rows; i++)
        {
            glm::vec3 p0 = { (i - rows / 2.0f) * spacing, 0.0f, -(cols / 2.0f) * spacing };
            glm::vec3 p1 = { (i - rows / 2.0f) * spacing, 0.0f, (cols / 2.0f) * spacing };
            DrawLines(p0, p1, color);
        }
        for (int j = 0; j <= cols; j++)
        {
            glm::vec3 p0 = { -(rows / 2.0f) * spacing, 0.0f, (j - cols / 2.0f) * spacing };
            glm::vec3 p1 = { (rows / 2.0f) * spacing, 0.0f, (j - cols / 2.0f) * spacing };
            DrawLines(p0, p1, color);
        }
    }

    void RendererMX::BeginPick(const EditorCamera& camera)
    {
        glm::mat4 viewProj = camera.GetViewProjection();

        s_Data->PickShader->Bind();
        s_Data->PickShader->SetMat4("u_ViewProjection", viewProj);
    }

    void RendererMX::EndPick()
    {
    }

    void RendererMX::DrawMeshToPickBuffer(MXMesh& mesh, TransformComponent& tc, int entityID)
    {
        s_Data->PickShader->Bind();
        s_Data->PickShader->SetMat4("u_ModelMatrix", tc.GetTransform());
        s_Data->PickShader->SetInt("u_EntityID", entityID);
        // VAO
        Ref<VertexArray> pickVertexArray = VertexArray::Create();

        auto& geometryHandler = mesh.getGeometryHandler();
        for (auto mesh : geometryHandler->getMeshes())
        {
            mx::MeshStreamPtr stream = mesh->getStream("position", 0);
            mx::MeshFloatBuffer& attributeData = stream->getData();
            uint32_t stride = stream->getStride();

            // VBO
            const float* bufferData = &attributeData[0];
            size_t bufferSize = attributeData.size() * sizeof(float);
            Ref<VertexBuffer> pickVertexBuffer = VertexBuffer::Create((void*)bufferData, bufferSize);
            pickVertexBuffer->SetLayout({
                { ShaderDataType::Float3, "a_Position" },
            });
            pickVertexArray->AddVertexBuffer(pickVertexBuffer);

            // IBO
            for (size_t i = 0; i < mesh->getPartitionCount(); i++)
            {
                mx::MeshPartitionPtr geom = mesh->getPartition(i);
                mx::MeshIndexBuffer& indexData = geom->getIndices();
                Ref<IndexBuffer> pickIndexBuffer = IndexBuffer::Create(indexData.data(), indexData.size());
                pickVertexArray->SetIndexBuffer(pickIndexBuffer);
                RenderCommand::DrawIndexed(pickVertexArray, indexData.size());
            }
        }
    }

    void RendererMX::invalidateShadowMap()
    {
        if (s_Data->_shadowMap)
        {
            s_Data->_imageHandler->releaseRenderResources(s_Data->_shadowMap);
            s_Data->_shadowMap = nullptr;
        }
    }

    void RendererMX::GammaCorrection(Ref<FrameBuffer> frameBuffer)
    {
        Ref<Texture2D> originalImage = Texture2D::Create(frameBuffer);
        originalImage->Bind(0);

        s_Data->GammaShader->Bind();
        RenderCommand::DrawIndexed(s_Data->GammaVertexArray, 6);
    }

}
