#ifndef RenderBuiltinResources_hpp
#define RenderBuiltinResources_hpp

#include "Render/RenderScene.hpp"
#include "Render/IBL/IBLResources.hpp"

// Renderer-owned built-in resources that are reused across frames and render passes.
struct RenderBuiltinResources {
    std::shared_ptr<RenderMeshResource> screen_quad;
    std::shared_ptr<RenderMeshResource> point_light_inst_mesh;

    // Split-Sum IBL 预计算结果（启动时构建一次，运行时由光照 pass 绑定）。
    IBLResources ibl;

    void reset() {
        screen_quad.reset();
        point_light_inst_mesh.reset();
    }
};

#endif // !RenderBuiltinResources_hpp
