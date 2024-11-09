#pragma once

#include "test/MXLight.h"
#include "test/MXMesh.h"

#include <MaterialXRender/Camera.h>

namespace Hazel {

	namespace mx = MaterialX;

	using MXCameraPtr = std::shared_ptr<class MXCamera>;

	class MXCamera
	{
	public:
		static MXCameraPtr create()
		{
			return std::make_shared<MXCamera>();
		}

		MXCamera();

		void initCamera();
		void updateCameras(MXMeshPtr mesh, MXLightPtr light);

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
	};

}