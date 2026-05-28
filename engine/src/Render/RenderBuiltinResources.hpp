#ifndef RenderBuiltinResources_hpp
#define RenderBuiltinResources_hpp

#include "Render/RenderScene.hpp"

// Renderer-owned built-in resources that are reused across frames and render passes.
struct RenderBuiltinResources {
    std::shared_ptr<RenderMeshResource> screen_quad;
    std::shared_ptr<RenderMeshResource> point_light_inst_mesh;

    void reset() {
        screen_quad.reset();
        point_light_inst_mesh.reset();
    }
};

#endif // !RenderBuiltinResources_hpp
