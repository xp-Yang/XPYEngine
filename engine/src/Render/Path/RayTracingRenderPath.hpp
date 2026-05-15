#ifndef RayTracingRenderPath_hpp
#define RayTracingRenderPath_hpp

#include "Render/Graph/RenderGraph.hpp"
#include "RenderPath.hpp"

class RayTracingRenderPath : public RenderPath {
public:
    RayTracingRenderPath();
    void render(RenderSourceData& render_source_data) override;
    void resizeRenderTargets(const Vec2& pixel_size) override;
    bool readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4]) override;

protected:
    std::unique_ptr<RenderPass> m_ray_tracing_pass;
    std::unique_ptr<RenderPass> m_combine_pass;
    RenderGraph m_render_graph;
};

#endif // !RayTracingRenderPath_hpp
