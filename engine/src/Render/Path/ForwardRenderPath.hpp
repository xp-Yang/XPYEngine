#ifndef ForwardRenderPath_hpp
#define ForwardRenderPath_hpp

#include "Render/Graph/RenderGraph.hpp"
#include "RenderPath.hpp"

class RenderSystem;
class ForwardRenderPath : public RenderPath {
public:
    ForwardRenderPath(RenderSystem* render_system);
    void render(RenderSourceData& render_source_data) override;
    void resizeRenderTargets(const Vec2& pixel_size) override;
    RhiTexture* renderGraphTextureOf(const std::string& resource_name) override;
    bool readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4]) override;
    std::vector<std::string> renderGraphResourceNames() const override;
    std::string renderGraphDebugDump() const override;
    std::string renderGraphExecutionDump() const override;

protected:
    RenderSystem* ref_render_system{ nullptr };
    RenderGraph m_render_graph;
};

#endif // !ForwardRenderPath_hpp
