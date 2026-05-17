#ifndef RayTracingRenderPath_hpp
#define RayTracingRenderPath_hpp

#include "Render/Graph/RenderGraph.hpp"
#include "RenderPath.hpp"

class RayTracingRenderPath : public RenderPath {
public:
    RayTracingRenderPath();
    void render(RenderSourceData& render_source_data) override;
    void resizeRenderTargets(const Vec2& pixel_size) override;
    RhiTexture* renderGraphTextureOf(const std::string& resource_name) override;
    bool readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4]) override;
    std::vector<std::string> renderGraphResourceNames() const override;
    std::vector<RenderGraphResourceDebugInfo> renderGraphResourceDebugInfos() const override;
    std::string renderGraphDebugDump() const override;
    std::string renderGraphExecutionDump() const override;

protected:
    std::unique_ptr<RenderPass> m_ray_tracing_pass;
    std::unique_ptr<RenderPass> m_final_pass;
    RenderGraph m_render_graph;
};

#endif // !RayTracingRenderPath_hpp
