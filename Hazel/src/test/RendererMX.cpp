#include "test/RendererMX.h"

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

        //glfwInit();
        //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //s_Data->_glfwWindow = glfwCreateWindow(1920, 1080, "Test", NULL, NULL);
        //glfwMakeContextCurrent(s_Data->_glfwWindow);
        //gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        // Initialize the standard libraries and color/unit management.
        loadStandardLibraries();

        // Initialize image handler.
        s_Data->_imageHandler = s_Data->_renderPipeline->createImageHandler();
        s_Data->_imageHandler->setSearchPath(s_Data->_searchPath);

        s_Data->_mesh->createGeometryHandler();

        s_Data->_renderPipeline->initFramebuffer(1920.0f, 1080.0f, nullptr);

        s_Data->_light->createEnvGeometryHandler();

        // Initialize environment light.
        s_Data->_light->loadEnvironmentLight();

        // Initialize camera.
        s_Data->_camera->initCamera();

        // Load the requested material document.
        s_Data->_mesh->loadDocument(s_Data->_stdLib);


        // Quad
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
    }

    void RendererMX::mainloop()
    {
        while (!glfwWindowShouldClose(s_Data->_glfwWindow))
        {
            draw_contents();
            glfwSwapBuffers(s_Data->_glfwWindow);
            glfwPollEvents();
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

    void RendererMX::draw_contents()
    {
        s_Data->_camera->updateCameras(s_Data->_mesh, s_Data->_light);

        mx::checkGlErrors("before viewer render");

        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Render the current frame.
        try
        {
            s_Data->_renderPipeline->renderFrame(s_Data->_colorTexture,
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
