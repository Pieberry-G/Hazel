#include "test/RendererMX.h"

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
        RendererMX::s_Data = new RendererMXData(mx::FileSearchPath("../../assets/MaterialX"),
            { "libraries" },
            mx::DEFAULT_SCREEN_COLOR_SRGB);

        //glfwInit();
        //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        //RendererMX::s_Data->_glfwWindow = glfwCreateWindow(1920, 1080, "Test", NULL, NULL);
        //glfwMakeContextCurrent(RendererMX::s_Data->_glfwWindow);
        //gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        // Initialize the standard libraries and color/unit management.
        loadStandardLibraries();

        // Initialize image handler.
        RendererMX::s_Data->_imageHandler = RendererMX::s_Data->_renderPipeline->createImageHandler();
        RendererMX::s_Data->_imageHandler->setSearchPath(RendererMX::s_Data->_searchPath);

        RendererMX::s_Data->_mesh->createGeometryHandler();

        RendererMX::s_Data->_renderPipeline->initFramebuffer(1920.0f, 1080.0f, nullptr);

        RendererMX::s_Data->_light->createEnvGeometryHandler();

        // Initialize environment light.
        RendererMX::s_Data->_light->loadEnvironmentLight();

        // Initialize camera.
        RendererMX::s_Data->_camera->initCamera();

        // Load the requested material document.
        RendererMX::s_Data->_mesh->loadDocument(RendererMX::s_Data->_stdLib);
    }

    void RendererMX::mainloop()
    {
        while (!glfwWindowShouldClose(RendererMX::s_Data->_glfwWindow))
        {
            draw_contents();
            glfwSwapBuffers(RendererMX::s_Data->_glfwWindow);
            glfwPollEvents();
        }
    }

    void RendererMX::initContext(mx::GenContext& context)
    {
        // Initialize search path.
        context.registerSourceCodeSearchPath(RendererMX::s_Data->_searchPath);

        // Initialize color management.
        mx::DefaultColorManagementSystemPtr cms = mx::DefaultColorManagementSystem::create(context.getShaderGenerator().getTarget());
        cms->loadLibrary(RendererMX::s_Data->_stdLib);
        context.getShaderGenerator().setColorManagementSystem(cms);

        // Initialize unit management.
        mx::UnitSystemPtr unitSystem = mx::UnitSystem::create(context.getShaderGenerator().getTarget());
        unitSystem->loadLibrary(RendererMX::s_Data->_stdLib);
        unitSystem->setUnitConverterRegistry(RendererMX::s_Data->_unitRegistry);
        context.getShaderGenerator().setUnitSystem(unitSystem);
        context.getOptions().targetDistanceUnit = "meter";
    }

    void RendererMX::loadStandardLibraries()
    {
        // Initialize the standard library.
        try
        {
            RendererMX::s_Data->_stdLib = mx::createDocument();
            RendererMX::s_Data->_xincludeFiles = mx::loadLibraries(RendererMX::s_Data->_libraryFolders, RendererMX::s_Data->_searchPath, RendererMX::s_Data->_stdLib);
            if (RendererMX::s_Data->_xincludeFiles.empty())
            {
                std::cerr << "Could not find standard data libraries on the given search path: " << RendererMX::s_Data->_searchPath.asString() << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to load standard data libraries: " << e.what() << std::endl;
            return;
        }

        // Initialize unit management.
        mx::UnitTypeDefPtr distanceTypeDef = RendererMX::s_Data->_stdLib->getUnitTypeDef("distance");
        RendererMX::s_Data->_distanceUnitConverter = mx::LinearUnitConverter::create(distanceTypeDef);
        RendererMX::s_Data->_unitRegistry->addUnitConverter(distanceTypeDef, RendererMX::s_Data->_distanceUnitConverter);
        mx::UnitTypeDefPtr angleTypeDef = RendererMX::s_Data->_stdLib->getUnitTypeDef("angle");
        mx::LinearUnitConverterPtr angleConverter = mx::LinearUnitConverter::create(angleTypeDef);
        RendererMX::s_Data->_unitRegistry->addUnitConverter(angleTypeDef, angleConverter);

        // Create the list of supported distance units.
        auto unitScales = RendererMX::s_Data->_distanceUnitConverter->getUnitScale();
        RendererMX::s_Data->_distanceUnitOptions.resize(unitScales.size());
        for (auto unitScale : unitScales)
        {
            int location = RendererMX::s_Data->_distanceUnitConverter->getUnitAsInteger(unitScale.first);
            RendererMX::s_Data->_distanceUnitOptions[location] = unitScale.first;
        }

        // Initialize the generator contexts.
        initContext(RendererMX::s_Data->_genContext);
    }

    void RendererMX::renderScreenSpaceQuad(mx::MaterialPtr material)
    {
        material->bindMesh(RendererMX::s_Data->_light->getQuadMesh());
        material->drawPartition(RendererMX::s_Data->_light->getQuadMesh()->getPartition(0));
    }

    void RendererMX::draw_contents()
    {
        RendererMX::s_Data->_camera->updateCameras(RendererMX::s_Data->_mesh, RendererMX::s_Data->_light);

        mx::checkGlErrors("before viewer render");

        // Set the requested background color.
        //glClearColor(RendererMX::s_Data->_screenColor[0], RendererMX::s_Data->_screenColor[1], RendererMX::s_Data->_screenColor[2], 1.0f);
        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Render the current frame.
        try
        {
            RendererMX::s_Data->_renderPipeline->renderFrame(RendererMX::s_Data->_colorTexture,
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
        if (RendererMX::s_Data->_shadowMap)
        {
            RendererMX::s_Data->_imageHandler->releaseRenderResources(RendererMX::s_Data->_shadowMap);
            RendererMX::s_Data->_shadowMap = nullptr;
        }
    }

}
