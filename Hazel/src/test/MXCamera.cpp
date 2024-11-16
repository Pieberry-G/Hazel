#include "test/MXCamera.h"
#include "test/RendererMX.h"

namespace
{
    const mx::Vector3 DEFAULT_CAMERA_POSITION(0.0f, 0.0f, 5.0f);
    const float DEFAULT_CAMERA_VIEW_ANGLE = 45.0f;
    const float DEFAULT_CAMERA_ZOOM = 1.0f;

    const std::string DIR_LIGHT_NODE_CATEGORY = "directional_light";
    const float ORTHO_VIEW_DISTANCE = 1000.0f;
    const float ORTHO_PROJECTION_HEIGHT = 1.8f;

    const float IDEAL_MESH_SPHERE_RADIUS = 2.0f;

    const float PI = std::acos(-1.0f);

} // anonymous namespace

namespace Hazel {

    MXCamera::MXCamera() :
        _cameraPosition(DEFAULT_CAMERA_POSITION),
        _cameraUp(0.0f, 1.0f, 0.0f),
        _cameraViewAngle(DEFAULT_CAMERA_VIEW_ANGLE),
        _cameraNearDist(0.05f),
        _cameraFarDist(5000.0f),
        _cameraZoom(DEFAULT_CAMERA_ZOOM),
        _userTranslationActive(false),
        _viewCamera(mx::Camera::create()),
        _envCamera(mx::Camera::create()),
        _shadowCamera(mx::Camera::create())
    {
    }

    void MXCamera::UpdateCameras(MXMeshPtr mesh, MXLightPtr light)
    {
        //const mx::Vector3 translation = RendererMX::s_Data->_mesh->getMeshTranslation();
        //const mx::Vector3 rotation = RendererMX::s_Data->_mesh->getMeshRotation();
        //const mx::Vector3 scale = RendererMX::s_Data->_mesh->getMeshScale();

        const mx::Vector3 translation = { 0, 0, 0 };
        const mx::Vector3 rotation = { 0, 0, 0 };
        const mx::Vector3 scale = { 1, 1, 1 };

        mx::Matrix44 meshRotation = mx::Matrix44::createRotationZ(rotation[2] / 180.0f * PI) *
            mx::Matrix44::createRotationY(rotation[1] / 180.0f * PI) *
            mx::Matrix44::createRotationX(rotation[0] / 180.0f * PI);

        _viewCamera->setWorldMatrix(mx::Matrix44::createScale(scale) * meshRotation * mx::Matrix44::createTranslation(translation));

        _envCamera->setWorldMatrix(mx::Matrix44::createScale(mx::Vector3(300.0f)));
        _envCamera->setViewMatrix(_viewCamera->getViewMatrix());
        _envCamera->setProjectionMatrix(_viewCamera->getProjectionMatrix());

        mx::NodePtr dirLight = light->getLightHandler()->getFirstLightOfCategory(DIR_LIGHT_NODE_CATEGORY);
        if (dirLight)
        {
            mx::Vector3 sphereCenter = (RendererMX::s_Data->_mesh->getGeometryHandler()->getMaximumBounds() + RendererMX::s_Data->_mesh->getGeometryHandler()->getMinimumBounds()) * 0.5;
            float r = (sphereCenter - RendererMX::s_Data->_mesh->getGeometryHandler()->getMinimumBounds()).getMagnitude();
            _shadowCamera->setWorldMatrix(meshRotation * mx::Matrix44::createTranslation(-sphereCenter));
            _shadowCamera->setProjectionMatrix(mx::Camera::createOrthographicMatrixZP(-r, r, -r, r, 0.0f, r * 2.0f));
            mx::ValuePtr value = dirLight->getInputValue("direction");
            if (value->isA<mx::Vector3>())
            {
                mx::Vector3 dir = mx::Matrix44::createRotationY(RendererMX::s_Data->_light->getLightRotation() / 180.0f * PI).transformVector(value->asA<mx::Vector3>());
                _shadowCamera->setViewMatrix(mx::Camera::createViewMatrix(dir * -r, mx::Vector3(0.0f), _cameraUp));
            }
        }
    }

}