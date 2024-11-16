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
		MXCamera();

		void UpdateCameras(MXMeshPtr mesh, MXLightPtr light);

		mx::CameraPtr GetViewCamera() const { return _viewCamera; }
		mx::CameraPtr GetEnvCamera() const { return _envCamera; }
		mx::CameraPtr GetShadowCamera() const { return _shadowCamera; }

		void SetCameraPosition(const mx::Vector3& position) { _cameraPosition = position; }
		void SetCameraTarget(const mx::Vector3& target) { _cameraTarget = target; }
		void SetCameraViewAngle(float angle) { _cameraViewAngle = angle; }
		void SetCameraZoom(float zoom) { _cameraZoom = zoom; }

	private:
		mx::Vector3 _cameraPosition;
		mx::Vector3 _cameraTarget;
		mx::Vector3 _cameraUp;
		float _cameraViewAngle;
		float _cameraNearDist;
		float _cameraFarDist;
		float _cameraZoom;

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