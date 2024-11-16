#pragma once

#include "test/MXMesh.h"
#include "test/MXLight.h"
#include "test/MXCamera.h"
#include "test/RenderPipeline.h"
#include "test/RenderPipelineGL.h"

#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/VertexArray.h"
#include "Hazel/Renderer/FrameBuffer.h"
#include "Hazel/Renderer/EditorCamera.h"

#include <MaterialXRender/ShaderMaterial.h>
#include <MaterialXRender/ImageHandler.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace mx = MaterialX;

namespace Hazel {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;

		// Editor-only
		int EntityID;
	};

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
			_drawEnvironment(false),
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

			_renderPipeline = CreateRef<GLRenderPipeline>();
			_mesh = CreateRef<MXMesh>();
			_light = CreateRef<MXLight>();
			_camera = CreateRef<MXCamera>();
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

		Ref<VertexArray> GammaVertexArray;
		Ref<Shader> GammaShader;

		// Line
		Ref<Shader> LineShader;
		Ref<VertexArray> LineVertexArray;
		Ref<VertexBuffer> LineVertexBuffer;
		uint32_t LineVertexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;
		float LineWidth = 2.0f;
	};

	class RendererMX
	{
	public:
		static void Init();
		
		static void BeginScene(const EditorCamera& camera);
		static void EndScene();

		static void draw_contents();
		static void DrawLines(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);
		static void DrawGroundPlane(int rows, int cols, float spacing = 1.0f);

		static void renderScreenSpaceQuad(mx::MaterialPtr material);
		
		static void initContext(mx::GenContext& context);
		static void loadStandardLibraries();
		static void invalidateShadowMap();
	
		static void GammaCorrection(Ref<FrameBuffer> frameBuffer);
	public:
		static RendererMXData* s_Data;
	};

}