#ifndef DeferredRenderPath_hpp
#define DeferredRenderPath_hpp

#include "Render/Graph/RenderGraph.hpp"
#include "RenderPath.hpp"

class RenderSystem;
class DeferredRenderPath : public RenderPath {
public:
    DeferredRenderPath(RenderSystem* render_system);

    void render(RenderSourceData& render_source_data) override;

    RhiTexture* renderGraphTextureOf(const std::string& resource_name) override;
    bool readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4]) override;

    std::vector<std::string> renderGraphResourceNames() const override;
    std::vector<RenderGraphResourceDebugInfo> renderGraphResourceDebugInfos() const override;
    std::string renderGraphDebugDump() const override;
    std::string renderGraphExecutionDump() const override;

    void resizeRenderTargets(const Vec2& pixel_size) override;

protected:
    RenderSystem* ref_render_system{ nullptr };
    RenderGraph m_render_graph;
};

#endif // !DeferredRenderPath_hpp
