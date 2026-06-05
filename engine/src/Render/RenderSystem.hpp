#ifndef RenderSystem_hpp
#define RenderSystem_hpp

#include "RenderParams.hpp"
#include "Path/RenderPath.hpp"
#include "Render/RenderBuiltinResources.hpp"
#include "Render/RenderFrameData.hpp"
#include "Render/RenderScene.hpp"
#include "Render/RHI/rhi.hpp"

class Scene;
class RenderSystem
{
public:
    RenderSystem();
    RenderParams &renderParams();
    void onUpdate(std::shared_ptr<Scene> scene);

    GL_HANDLE renderGraphTextureOf(const std::string &resource_name);
    bool readRenderGraphPixelRGBAOf(const std::string &resource_name, int x, int y, unsigned char out_rgba[4]);

    std::vector<std::string> renderGraphResourceNames() const;
    std::vector<RenderGraphResourceDebugInfo> renderGraphResourceDebugInfos() const;
    std::string renderGraphDebugDump() const;
    std::string renderGraphExecutionDump() const;

    /** Reallocate all offscreen FBOs for current `renderParams().render_resolution`. Call after changing preset. */
    void rebuildRenderTargets();

protected:
    void initializeRenderResources();
    void buildIBLResources(const std::string& asset_dir);
    void syncRenderSceneChanges(Scene& scene);
    void rebuildRenderSceneFromScene(Scene& scene);
    void rebuildObjectRenderProxy(GObject& object);
    void updateSkinnedMeshSections();
    void buildRenderFrameData(Scene& scene);
    void updateMainCameraCulling();

private:
    RenderParams m_render_params;

    std::shared_ptr<RenderPath> m_forward_path;
    std::shared_ptr<RenderPath> m_deferred_path;
    std::shared_ptr<RenderPath> m_curr_path;

    RenderScene m_render_scene;
    RenderFrameData m_frame_data;
    RenderBuiltinResources m_builtin_resources;

    bool m_initialized{false};
};

#endif // !RenderSystem_hpp
