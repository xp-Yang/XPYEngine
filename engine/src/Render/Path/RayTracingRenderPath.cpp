#include "RayTracingRenderPath.hpp"

#include "../Pass/RayTracingPass.hpp"
#include "../Pass/FinalPass.hpp"

RayTracingRenderPath::RayTracingRenderPath()
{
    m_ray_tracing_pass = std::make_unique<RayTracingPass>();
    m_final_pass = std::make_unique<FinalPass>();
}

void RayTracingRenderPath::render(RenderSourceData& render_source_data)
{
    m_render_graph.reset();

    m_render_graph.addPass(RenderPass::Type::RayTracing, m_ray_tracing_pass.get())
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::SceneColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::SceneDepth, RhiTexture::Format::DEPTH);

    m_render_graph.addPass(RenderPass::Type::Final, m_final_pass.get())
        .readWrite(RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth)
        .target(RGTarget::ScreenFrameBuffer, RenderTargetType::ScreenFrameBuffer);

    m_render_graph.markOutput(RGResource::SceneColor);
    m_render_graph.compile();
    m_render_graph.execute(render_source_data);
}

void RayTracingRenderPath::resizeRenderTargets(const Vec2 &pixel_size)
{
    m_render_graph.setFrameSize(pixel_size);
}

bool RayTracingRenderPath::readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4])
{
    return m_render_graph.readPixelRGBAOf(resource_name, x, y, out_rgba);
}
