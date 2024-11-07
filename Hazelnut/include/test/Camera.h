#pragma once

#include "test/Light.h"
#include "test/Mesh.h"

#include <MaterialXRender/Camera.h>

namespace Hazel {

	namespace mx = MaterialX;

	using CameraPtr = std::shared_ptr<class Camera>;
	class Viewer;

	class Camera
	{
	public:
		static CameraPtr create(Viewer* viewer)
		{
			return std::make_shared<Camera>(viewer);
		}

		Camera(Viewer* viewer);

		void initCamera();
		void updateCameras(MeshPtr mesh, LightPtr light);

		mx::CameraPtr getViewCamera() const { return _viewCamera; }
		mx::CameraPtr getEnvCamera() const { return _envCamera; }
		mx::CameraPtr getShadowCamera() const { return _shadowCamera; }

		void setCameraPosition(const mx::Vector3& position) { _cameraPosition = position; }
		void setCameraTarget(const mx::Vector3& target) { _cameraTarget = target; }
		void setCameraViewAngle(float angle) { _cameraViewAngle = angle; }
		void setCameraZoom(float zoom) { _cameraZoom = zoom; }
	
	private:
		mx::Vector3 _cameraPosition;
		mx::Vector3 _cameraTarget;
		mx::Vector3 _cameraUp;
		float _cameraViewAngle;
		float _cameraNearDist;
		float _cameraFarDist;
		float _cameraZoom;

		bool _userCameraEnabled;
		mx::Vector3 _userTranslation;
		mx::Vector3 _userTranslationStart;
		bool _userTranslationActive;
		mx::Vector2 _userTranslationPixel;

		// Cameras
		mx::CameraPtr _viewCamera;
		mx::CameraPtr _envCamera;
		mx::CameraPtr _shadowCamera;

	public:
		Viewer* _viewer;
	};

}