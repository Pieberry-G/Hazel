#pragma once

#include "test/Mesh.h"
#include "test/Light.h"
#include "test/Camera.h"
#include "test/RenderPipeline.h"

#include <MaterialXRender/ShaderMaterial.h>
#include <MaterialXRender/GeometryHandler.h>
#include <MaterialXRender/ImageHandler.h>

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

	class Viewer
	{
		friend class Mesh;
		friend class Light;
		friend class Camera;
		friend class RenderPipeline;
		friend class GLRenderPipeline;
	public:
		Viewer(const mx::FileSearchPath& searchPath,
			const mx::FilePathVec& libraryFolders,
			const mx::Color3& screenColor);
		~Viewer() { }

		// Initialize the viewer for rendering.
		void initialize();

		// Render in a continuous loop.
		void mainloop();

		// Set the method for specular environment rendering.
		void setSpecularEnvironmentMethod(mx::HwSpecularEnvironmentMethod method)
		{
			_genContext.getOptions().hwSpecularEnvironmentMethod = method;
		}

		// Enable or disable shadow maps.
		void setShadowMapEnable(bool enable)
		{
			_genContext.getOptions().hwShadowMap = enable;
		}

		// Enable or disable drawing environment as the background.
		void setDrawEnvironment(bool enable)
		{
			_drawEnvironment = enable;
		}

		// Set the modifiers to be applied to loaded documents.
		void setDocumentModifiers(const DocumentModifiers& modifiers)
		{
			_modifiers = modifiers;
		}

		// Return the active image handler.
		mx::ImageHandlerPtr getImageHandler() const
		{
			return _imageHandler;
		}

	private:
		void draw_contents();
		void renderScreenSpaceQuad(mx::MaterialPtr material);
		
		void initContext(mx::GenContext& context);
		void loadStandardLibraries();
		void invalidateShadowMap();


	private:
		MeshPtr _mesh;
		LightPtr _light;
		CameraPtr _camera;
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

}

extern const mx::Vector3 DEFAULT_CAMERA_POSITION;
extern const float DEFAULT_CAMERA_VIEW_ANGLE;
extern const float DEFAULT_CAMERA_ZOOM;