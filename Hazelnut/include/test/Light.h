#pragma once

#include <MaterialXRender/LightHandler.h>
#include <MaterialXRender/ShaderMaterial.h>
#include <MaterialXRender/GeometryHandler.h>

namespace Hazel {

	namespace mx = MaterialX;

	using LightPtr = std::shared_ptr<class Light>;
	class Viewer;

	class Light
	{
	public:
		static LightPtr create(Viewer* viewer)
		{
			return std::make_shared<Light>(viewer);
		}

		Light(Viewer* viewer);

		void createEnvGeometryHandler();
		void loadEnvironmentLight();
		mx::MaterialPtr getEnvironmentMaterial();

		mx::LightHandlerPtr getLightHandler() const { return _lightHandler; }
		float getLightRotation() const { return _lightRotation; }
		mx::MeshPtr getQuadMesh() const { return _quadMesh; }
		mx::FilePath getLightRigFilename() const { return _lightRigFilename; }
		mx::DocumentPtr getLightRigDoc() const { return _lightRigDoc; }
		mx::GeometryHandlerPtr getEnvGeometryHandler() const { return _envGeometryHandler; }

		void setEnvSampleCount(int count) { _lightHandler->setEnvSampleCount(count); }
		void setEnvLightIntensity(float intensity) { _lightHandler->setEnvLightIntensity(intensity); }

		// Set the rotation of the lighting environment about the Y axis.
		void setLightRotation(float rotation) { _lightRotation = rotation; }

	private:
		// Split the given radiance map into indirect and direct components,
		// returning a new indirect map and directional light document.
		void splitDirectLight(mx::ImagePtr envRadianceMap, mx::ImagePtr& indirectMap, mx::DocumentPtr& dirLightDoc);
	
	private:
		mx::FilePath _envRadianceFilename;

		// Lighting information
		mx::FilePath _lightRigFilename;
		mx::DocumentPtr _lightRigDoc;
		float _lightRotation;

		// Light processing options
		bool _normalizeEnvironment;
		bool _splitDirectLight;
		bool _generateReferenceIrradiance;
		bool _saveGeneratedLights;

		// Resource handlers
		mx::LightHandlerPtr _lightHandler;

		// Supporting materials and geometry.
		mx::GeometryHandlerPtr _envGeometryHandler;
		mx::MaterialPtr _envMaterial;
		mx::MeshPtr _quadMesh;

	public:
		Viewer* _viewer;
	};

}