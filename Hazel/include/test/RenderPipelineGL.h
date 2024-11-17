#pragma once

#include "test/RenderPipeline.h"

namespace Hazel {

    class GLRenderPipeline : public RenderPipeline
    {
    public:
        ~GLRenderPipeline() {}

        void initialize(void* metal_device, void* metal_cmd_queue) override;

        mx::ImageHandlerPtr createImageHandler() override;
        mx::MaterialPtr     createMaterial() override;
        void updatePrefilteredMap() override;
        void renderFrame(MXMesh& mesh, void* color_texture, int shadowMapSize, const char* dirLightNodeCat) override;

    public:
        GLRenderPipeline() = default;

    protected:
        mx::ImagePtr getShadowMap(MXMesh& mesh, int shadowMapSize) override;
    };

}
