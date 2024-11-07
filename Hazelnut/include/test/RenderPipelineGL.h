#pragma once

#include "test/RenderPipeline.h"

namespace Hazel {

    class Viewer;
    using GLRenderPipelinePtr = std::shared_ptr<class GLRenderPipeline>;

    class GLRenderPipeline : public RenderPipeline
    {
    public:
        ~GLRenderPipeline() { }

        static GLRenderPipelinePtr create(Viewer* viewer)
        {
            return std::make_shared<GLRenderPipeline>(viewer);
        }

        void initialize(void* metal_device, void* metal_cmd_queue) override;

        void initFramebuffer(int width, int height,
            void* color_texture) override;
        void resizeFramebuffer(int width, int height,
            void* color_texture) override;

        mx::ImageHandlerPtr createImageHandler() override;
        mx::MaterialPtr     createMaterial() override;
        void updatePrefilteredMap() override;
        void renderFrame(void* color_texture, int shadowMapSize, const char* dirLightNodeCat) override;

    public:
        GLRenderPipeline(Viewer* viewerPtr);

    protected:
        mx::ImagePtr getShadowMap(int shadowMapSize) override;
    };

}
