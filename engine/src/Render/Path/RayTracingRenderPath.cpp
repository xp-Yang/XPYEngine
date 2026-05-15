#include "RayTracingRenderPath.hpp"

#include "../Pass/RayTracingPass.hpp"
#include "../Pass/CombinePass.hpp"

RayTracingRenderPath::RayTracingRenderPath()
{
    m_ray_tracing_pass = std::make_unique<RayTracingPass>();
    m_combine_pass = std::make_unique<CombinePass>();
}

void RayTracingRenderPath::render(RenderSourceData& render_source_data)
{
    m_render_graph.reset();

    m_render_graph.addPass(RenderPass::Type::RayTracing, m_ray_tracing_pass.get())
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::SceneColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::SceneDepth, RhiTexture::Format::DEPTH);

    m_render_graph.addPass(RenderPass::Type::Combined, m_combine_pass.get())
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .read(RGResource::SceneColor)
        .read(RGResource::SceneDepth)
        .color(RGResource::FinalColor, RhiTexture::Format::RGB8)
        .depth(RGResource::FinalDepth, RhiTexture::Format::DEPTH)
        .target(RGTarget::Backbuffer, RenderTargetType::Backbuffer);

    m_render_graph.markOutput(RGResource::FinalColor);
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
