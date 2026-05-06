#ifndef RayTracingRenderPath_hpp
#define RayTracingRenderPath_hpp

#include "RenderPath.hpp"

class RayTracingRenderPath : public RenderPath {
public:
    RayTracingRenderPath();
    void prepareRenderSourceData(const std::shared_ptr<RenderSourceData>& render_source_data) override;
    void render() override;
    void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
    std::unique_ptr<RenderPass> m_ray_tracing_pass;
    std::unique_ptr<RenderPass> m_combine_pass;
};

#endif // !RayTracingRenderPath_hpp