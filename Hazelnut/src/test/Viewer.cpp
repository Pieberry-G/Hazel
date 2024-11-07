#include "test/Viewer.h"
#include "test/RenderPipeline.h"

#include "test/RenderPipelineGL.h"

#include <MaterialXRenderGlsl/GLUtil.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>

#include <MaterialXFormat/Util.h>

#include <iostream>

namespace Hazel {

    namespace
    {
        const int SHADOW_MAP_SIZE = 2048;
        const std::string DIR_LIGHT_NODE_CATEGORY = "directional_light";

    } // anonymous namespace

    Viewer::Viewer(const mx::FileSearchPath& searchPath,
        const mx::FilePathVec& libraryFolders,

        const mx::Color3& screenColor) :
        _searchPath(searchPath),
        _libraryFolders(libraryFolders),
        _shadowSoftness(1),
        _ambientOcclusionGain(0.6f),
        _genContext(mx::GlslShaderGenerator::create()),
        _unitRegistry(mx::UnitConverterRegistry::create()),
        _drawEnvironment(false),
        _outlineSelection(false),
        _renderTransparency(true),
        _renderDoubleSided(true),
        _colorTexture(nullptr),
        _screenColor(screenColor),
        _glfwWindow(nullptr)
    {
        //// Resolve input filenames, taking both the provided search path and
        //// current working directory into account.
        //mx::FileSearchPath localSearchPath = searchPath;
        //localSearchPath.append(mx::FilePath::getCurrentPath());
        //_materialFilename = localSearchPath.find(_materialFilename);
        //_meshFilename = localSearchPath.find(_meshFilename);
        //_envRadianceFilename = localSearchPath.find(_envRadianceFilename);

        // Set default Glsl generator options.
        _genContext.getOptions().targetColorSpaceOverride = "lin_rec709";
        _genContext.getOptions().fileTextureVerticalFlip = true;
        _genContext.getOptions().hwShadowMap = true;
        _genContext.getOptions().hwImplicitBitangents = false;

        _renderPipeline = GLRenderPipeline::create(this);
        _mesh = Mesh::create(this);
        _light = Light::create(this);
        _camera = Camera::create(this);
    }

    void Viewer::initialize()
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        _glfwWindow = glfwCreateWindow(1920, 1080, "Test", NULL, NULL);
        glfwMakeContextCurrent(_glfwWindow);
        gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

        // Initialize the standard libraries and color/unit management.
        loadStandardLibraries();

        // Initialize image handler.
        _imageHandler = _renderPipeline->createImageHandler();
        _imageHandler->setSearchPath(_searchPath);

        _mesh->createGeometryHandler();

        _renderPipeline->initFramebuffer(1920.0f, 1080.0f, nullptr);

        _light->createEnvGeometryHandler();

        // Initialize environment light.
        _light->loadEnvironmentLight();

        // Initialize camera.
        _camera->initCamera();

        // Load the requested material document.
        _mesh->loadDocument(_stdLib);
    }

    void Viewer::mainloop()
    {
        while (!glfwWindowShouldClose(_glfwWindow))
        {
            draw_contents();
            glfwSwapBuffers(_glfwWindow);
            glfwPollEvents();
        }
    }

    void Viewer::initContext(mx::GenContext& context)
    {
        // Initialize search path.
        context.registerSourceCodeSearchPath(_searchPath);

        // Initialize color management.
        mx::DefaultColorManagementSystemPtr cms = mx::DefaultColorManagementSystem::create(context.getShaderGenerator().getTarget());
        cms->loadLibrary(_stdLib);
        context.getShaderGenerator().setColorManagementSystem(cms);

        // Initialize unit management.
        mx::UnitSystemPtr unitSystem = mx::UnitSystem::create(context.getShaderGenerator().getTarget());
        unitSystem->loadLibrary(_stdLib);
        unitSystem->setUnitConverterRegistry(_unitRegistry);
        context.getShaderGenerator().setUnitSystem(unitSystem);
        context.getOptions().targetDistanceUnit = "meter";
    }

    void Viewer::loadStandardLibraries()
    {
        // Initialize the standard library.
        try
        {
            _stdLib = mx::createDocument();
            _xincludeFiles = mx::loadLibraries(_libraryFolders, _searchPath, _stdLib);
            if (_xincludeFiles.empty())
            {
                std::cerr << "Could not find standard data libraries on the given search path: " << _searchPath.asString() << std::endl;
            }
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to load standard data libraries: " << e.what() << std::endl;
            return;
        }

        // Initialize unit management.
        mx::UnitTypeDefPtr distanceTypeDef = _stdLib->getUnitTypeDef("distance");
        _distanceUnitConverter = mx::LinearUnitConverter::create(distanceTypeDef);
        _unitRegistry->addUnitConverter(distanceTypeDef, _distanceUnitConverter);
        mx::UnitTypeDefPtr angleTypeDef = _stdLib->getUnitTypeDef("angle");
        mx::LinearUnitConverterPtr angleConverter = mx::LinearUnitConverter::create(angleTypeDef);
        _unitRegistry->addUnitConverter(angleTypeDef, angleConverter);

        // Create the list of supported distance units.
        auto unitScales = _distanceUnitConverter->getUnitScale();
        _distanceUnitOptions.resize(unitScales.size());
        for (auto unitScale : unitScales)
        {
            int location = _distanceUnitConverter->getUnitAsInteger(unitScale.first);
            _distanceUnitOptions[location] = unitScale.first;
        }

        // Initialize the generator contexts.
        initContext(_genContext);
    }

    void Viewer::renderScreenSpaceQuad(mx::MaterialPtr material)
    {
        material->bindMesh(_light->getQuadMesh());
        material->drawPartition(_light->getQuadMesh()->getPartition(0));
    }

    void Viewer::draw_contents()
    {
        _camera->updateCameras(_mesh, _light);

        mx::checkGlErrors("before viewer render");

        // Set the requested background color.
        glClearColor(_screenColor[0], _screenColor[1], _screenColor[2], 1.0f);
        // Clear the screen.
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Render the current frame.
        try
        {
            _renderPipeline->renderFrame(_colorTexture,
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

    void Viewer::invalidateShadowMap()
    {
        if (_shadowMap)
        {
            _imageHandler->releaseRenderResources(_shadowMap);
            _shadowMap = nullptr;
        }
    }

}
