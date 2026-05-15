#ifndef RayTracingRenderPath_hpp
#define RayTracingRenderPath_hpp

#include "Render/Graph/RenderGraph.hpp"
#include "RenderPath.hpp"

class RayTracingRenderPath : public RenderPath {
public:
    RayTracingRenderPath();
    void prepareRenderSourceData(const std::shared_ptr<RenderSourceData>& render_source_data) override;
    void render() override;
    void resizeRenderTargets(const Vec2& pixel_size) override;

protected:
    std::unique_ptr<RenderPass> m_ray_tracing_pass;
    std::unique_ptr<RenderPass> m_combine_pass;
    RenderGraph m_render_graph;
};

#endif // !RayTracingRenderPath_hpp
