#include "test/Camera.h"
#include "test/Viewer.h"

const mx::Vector3 DEFAULT_CAMERA_POSITION(0.0f, 0.0f, 5.0f);
const float DEFAULT_CAMERA_VIEW_ANGLE = 45.0f;
const float DEFAULT_CAMERA_ZOOM = 1.0f;

namespace
{
    const std::string DIR_LIGHT_NODE_CATEGORY = "directional_light";
    const float ORTHO_VIEW_DISTANCE = 1000.0f;
    const float ORTHO_PROJECTION_HEIGHT = 1.8f;

    const float IDEAL_MESH_SPHERE_RADIUS = 2.0f;

    const float PI = std::acos(-1.0f);

} // anonymous namespace

namespace Hazel {

    Camera::Camera(Viewer* viewer) :
        _viewer(viewer),
        _cameraPosition(DEFAULT_CAMERA_POSITION),
        _cameraUp(0.0f, 1.0f, 0.0f),
        _cameraViewAngle(DEFAULT_CAMERA_VIEW_ANGLE),
        _cameraNearDist(0.05f),
        _cameraFarDist(5000.0f),
        _cameraZoom(DEFAULT_CAMERA_ZOOM),
        _userCameraEnabled(true),
        _userTranslationActive(false),
        _viewCamera(mx::Camera::create()),
        _envCamera(mx::Camera::create()),
        _shadowCamera(mx::Camera::create())
    {
    }

    void Camera::initCamera()
    {
        _viewCamera->setViewportSize(mx::Vector2(1920.0f, 1080.0f));

        // Disable user camera controls when non-centered views are requested.
        _userCameraEnabled = _cameraTarget == mx::Vector3(0.0) &&
            _viewer->_mesh->getMeshScale() == 1.0f;

        if (!_userCameraEnabled || _viewer->_mesh->getGeometryHandler()->getMeshes().empty())
        {
            return;
        }

        const mx::Vector3& boxMax = _viewer->_mesh->getGeometryHandler()->getMaximumBounds();
        const mx::Vector3& boxMin = _viewer->_mesh->getGeometryHandler()->getMinimumBounds();
        mx::Vector3 sphereCenter = (boxMax + boxMin) * 0.5;

        const mx::Vector3 rotation = _viewer->_mesh->getMeshRotation();
        float yRotation = rotation[1];
        mx::Matrix44 meshRotation = mx::Matrix44::createRotationZ(rotation[2] / 180.0f * PI) *
            mx::Matrix44::createRotationY(yRotation / 180.0f * PI) *
            mx::Matrix44::createRotationX(rotation[0] / 180.0f * PI);
        _viewer->_mesh->setMeshTranslation(-meshRotation.transformPoint(sphereCenter));
        _viewer->_mesh->setMeshScale(IDEAL_MESH_SPHERE_RADIUS / (sphereCenter - boxMin).getMagnitude());
    }

    void Camera::updateCameras(MeshPtr mesh, LightPtr light)
    {
        auto& createPerspectiveMatrix = mx::Camera::createPerspectiveMatrix;
        auto& createOrthographicMatrix = mx::Camera::createOrthographicMatrix;
        mx::Matrix44 viewMatrix, projectionMatrix;
        float aspectRatio = 1920.0f / 1080.0f;
        if (_cameraViewAngle != 0.0f)
        {
            viewMatrix = mx::Camera::createViewMatrix(_cameraPosition, _cameraTarget, _cameraUp);
            float fH = std::tan(_cameraViewAngle / 360.0f * PI) * _cameraNearDist;
            float fW = fH * aspectRatio;
            projectionMatrix = createPerspectiveMatrix(-fW, fW, -fH, fH, _cameraNearDist, _cameraFarDist);
        }
        else
        {
            viewMatrix = mx::Matrix44::createTranslation(mx::Vector3(0.0f, 0.0f, -ORTHO_VIEW_DISTANCE));
            float fH = ORTHO_PROJECTION_HEIGHT;
            float fW = fH * aspectRatio;
            projectionMatrix = createOrthographicMatrix(-fW, fW, -fH, fH, 0.0f, ORTHO_VIEW_DISTANCE + _cameraFarDist);
        }
        const mx::Vector3 translation = _viewer->_mesh->getMeshTranslation();
        const mx::Vector3 rotation = _viewer->_mesh->getMeshRotation();
        float scale = _viewer->_mesh->getMeshScale();
        float yRotation = rotation[1];
        mx::Matrix44 meshRotation = mx::Matrix44::createRotationZ(rotation[2] / 180.0f * PI) *
            mx::Matrix44::createRotationY(yRotation / 180.0f * PI) *
            mx::Matrix44::createRotationX(rotation[0] / 180.0f * PI);

        mx::Matrix44 arcball = mx::Matrix44::IDENTITY;
        if (_userCameraEnabled)
        {
            arcball = _viewCamera->arcballMatrix();
        }

        _viewCamera->setWorldMatrix(meshRotation *
            mx::Matrix44::createTranslation(translation + _userTranslation) *
            mx::Matrix44::createScale(mx::Vector3(scale * _cameraZoom)));
        _viewCamera->setViewMatrix(arcball * viewMatrix);
        _viewCamera->setProjectionMatrix(projectionMatrix);

        _envCamera->setWorldMatrix(mx::Matrix44::createScale(mx::Vector3(300.0f)));
        _envCamera->setViewMatrix(_viewCamera->getViewMatrix());
        _envCamera->setProjectionMatrix(_viewCamera->getProjectionMatrix());

        mx::NodePtr dirLight = light->getLightHandler()->getFirstLightOfCategory(DIR_LIGHT_NODE_CATEGORY);
        if (dirLight)
        {
            mx::Vector3 sphereCenter = (_viewer->_mesh->getGeometryHandler()->getMaximumBounds() + _viewer->_mesh->getGeometryHandler()->getMinimumBounds()) * 0.5;
            float r = (sphereCenter - _viewer->_mesh->getGeometryHandler()->getMinimumBounds()).getMagnitude();
            _shadowCamera->setWorldMatrix(meshRotation * mx::Matrix44::createTranslation(-sphereCenter));
            _shadowCamera->setProjectionMatrix(mx::Camera::createOrthographicMatrixZP(-r, r, -r, r, 0.0f, r * 2.0f));
            mx::ValuePtr value = dirLight->getInputValue("direction");
            if (value->isA<mx::Vector3>())
            {
                mx::Vector3 dir = mx::Matrix44::createRotationY(_viewer->_light->getLightRotation() / 180.0f * PI).transformVector(value->asA<mx::Vector3>());
                _shadowCamera->setViewMatrix(mx::Camera::createViewMatrix(dir * -r, mx::Vector3(0.0f), _cameraUp));
            }
        }
    }

}