#ifndef RenderSystem_hpp
#define RenderSystem_hpp

#include "RenderParams.hpp"
#include "Path/RenderPath.hpp"

class Scene;
class RenderSystem {
public:
    RenderSystem();
    RenderParams& renderParams();
    void onUpdate(std::shared_ptr<Scene> scene);
    unsigned int renderGraphTexture(const std::string& resource_name);
    bool readRenderGraphPixelRGBA(const std::string& resource_name, int x, int y, unsigned char out_rgba[4]);
    std::vector<std::string> renderGraphResourceNames() const;
    std::string renderGraphDebugDump() const;
    std::string renderGraphExecutionDump() const;

    /** Reallocate all offscreen FBOs for current `renderParams().render_resolution`. Call after changing preset. */
    void rebuildRenderTargets();

protected:
    void updateRenderSourceData(std::shared_ptr<Scene> scene);

private:
    RenderParams m_render_params;

    std::shared_ptr<RenderPath> m_forward_path;
    std::shared_ptr<RenderPath> m_deferred_path;
    std::shared_ptr<RenderPath> m_curr_path;

    std::shared_ptr<RenderSourceData> m_render_source_data;

    bool m_initialized{ false };
};

#endif // !RenderSystem_hpp
