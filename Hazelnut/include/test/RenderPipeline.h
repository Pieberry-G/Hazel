#pragma once

#include <MaterialXRender/ShaderMaterial.h>
#include <MaterialXRender/Camera.h>
#include <MaterialXRender/GeometryHandler.h>
#include <MaterialXRender/LightHandler.h>
#include <MaterialXRender/ImageHandler.h>
#include <MaterialXRender/Image.h>

#include <MaterialXCore/Value.h>
#include <MaterialXCore/Unit.h>

MATERIALX_NAMESPACE_BEGIN
using TextureBakerPtr = shared_ptr<class TextureBakerGlsl>;
MATERIALX_NAMESPACE_END

#include <memory>

namespace mx = MaterialX;

namespace Hazel {

    class Viewer;
    using RenderPipelinePtr = std::shared_ptr<class RenderPipeline>;

    class RenderPipeline
    {
    public:
        RenderPipeline() = delete;
        RenderPipeline(Viewer* viewer)
        {
            _viewer = viewer;
        }
        virtual ~RenderPipeline() { }

        virtual void initialize(void* device, void* command_queue) = 0;

        virtual mx::ImageHandlerPtr createImageHandler() = 0;
        virtual mx::MaterialPtr createMaterial() = 0;

        virtual void updatePrefilteredMap() = 0;

        virtual void renderFrame(void* color_texture, int shadowMapSize, const char* dirLightNodeCat) = 0;

        virtual void initFramebuffer(int width, int height,
            void* color_texture) = 0;
        virtual void resizeFramebuffer(int width, int height,
            void* color_texture) = 0;

        virtual mx::ImagePtr getShadowMap(int shadowMapSize) = 0;

    public:
        Viewer* _viewer;
    };


}