#pragma once

#include "test/MXMesh.h"
#include "test/MXLight.h"
#include "test/MXCamera.h"
#include "test/RenderPipeline.h"
#include "test/RenderPipelineGL.h"

#include <MaterialXRender/ShaderMaterial.h>
#include <MaterialXRender/ImageHandler.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace mx = MaterialX;

namespace Hazel {

	class DocumentModifiers
	{
	public:
		mx::StringMap remapElements;
		mx::StringSet skipElements;
		std::string filePrefixTerminator;
	};

	struct RendererMXData
	{
		RendererMXData(const mx::FileSearchPath& searchPath,
			const mx::FilePathVec& libraryFolders,
			const mx::Color3& screenColor) :
			_searchPath(searchPath),
			_libraryFolders(libraryFolders),
			_shadowSoftness(1),
			_ambientOcclusionGain(0.6f),
			_genContext(mx::GlslShaderGenerator::create()),
			_unitRegistry(mx::UnitConverterRegistry::create()),
			_drawEnvironment(true),
			_outlineSelection(false),
			_renderTransparency(true),
			_renderDoubleSided(true),
			_colorTexture(nullptr),
			_screenColor(screenColor),
			_glfwWindow(nullptr)
		{
			// Set default Glsl generator options.
			_genContext.getOptions().targetColorSpaceOverride = "lin_rec709";
			_genContext.getOptions().fileTextureVerticalFlip = true;
			_genContext.getOptions().hwShadowMap = true;
			_genContext.getOptions().hwImplicitBitangents = false;

			_renderPipeline = GLRenderPipeline::create();
			_mesh = MXMesh::create();
			_light = MXLight::create();
			_camera = MXCamera::create();
		}

		MXMeshPtr _mesh;
		MXLightPtr _light;
		MXCameraPtr _camera;
		RenderPipelinePtr _renderPipeline;

		mx::FileSearchPath _searchPath;
		mx::FilePathVec _libraryFolders;

		// Document management
		mx::DocumentPtr _stdLib;
		DocumentModifiers _modifiers;
		mx::StringSet _xincludeFiles;

		// Shadow mapping
		mx::MaterialPtr _shadowMaterial;
		mx::MaterialPtr _shadowBlurMaterial;
		mx::ImagePtr _shadowMap;
		unsigned int _shadowSoftness;

		// Ambient occlusion
		float _ambientOcclusionGain;

		// Resource handlers
		mx::ImageHandlerPtr _imageHandler;

		// Shader generator contexts
		mx::GenContext _genContext;

		// Unit registry
		mx::UnitConverterRegistryPtr _unitRegistry;

		// Viewing options
		bool _drawEnvironment;
		bool _outlineSelection;

		// Render options
		bool _renderTransparency;
		bool _renderDoubleSided;

		// Framebuffer Color Texture
		void* _colorTexture;

		// Scene options
		mx::StringVec _distanceUnitOptions;
		mx::LinearUnitConverterPtr _distanceUnitConverter;

		GLFWwindow* _glfwWindow;
		mx::Color3 _screenColor;
		int m_fbsize[2] = { 1920, 1080 };
	};

	class RendererMX
	{
	public:
		static void Init();
		static void mainloop();
		static void draw_contents();
		static void renderScreenSpaceQuad(mx::MaterialPtr material);
		
		static void initContext(mx::GenContext& context);
		static void loadStandardLibraries();
		static void invalidateShadowMap();
	
	public:
		static RendererMXData* s_Data;
	};

}