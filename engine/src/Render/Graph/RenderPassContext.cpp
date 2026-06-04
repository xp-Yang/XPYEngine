#include "RenderPassContext.hpp"

RenderPassContext::RenderPassContext(
    RenderGraph& graph,
    const RenderGraphPassNode& node,
    RenderScene& render_scene,
    RenderFrameData& frame_data,
    RenderBuiltinResources& builtin_resources)
    : m_graph(&graph)
    , m_node(&node)
    , m_render_scene(&render_scene)
    , m_frame_data(&frame_data)
    , m_builtin_resources(&builtin_resources)
{
}

RenderScene& RenderPassContext::renderScene() const
{
    return *m_render_scene;
}

RenderFrameData& RenderPassContext::frameData() const
{
    return *m_frame_data;
}

RenderBuiltinResources& RenderPassContext::builtinResources() const
{
    return *m_builtin_resources;
}

RhiTexture* RenderPassContext::texture(const std::string& resource_name) const
{
    return m_graph ? m_graph->textureOf(resource_name) : nullptr;
}

RhiFrameBuffer* RenderPassContext::frameBuffer(const std::string& resource_name) const
{
    return m_graph ? m_graph->frameBufferOf(resource_name) : nullptr;
}

RhiFrameBuffer* RenderPassContext::frameBufferOfTarget(const std::string& target_name) const
{
    const RenderGraphRenderTarget* target = m_graph->findRenderTarget(m_node->m_type, target_name);
    return target ? target->framebuffer.get() : nullptr;
}

RhiFrameBuffer* RenderPassContext::defaultFrameBuffer() const
{
    const RenderGraphRenderTarget* target = m_graph->findRenderTarget(m_node->m_type, RGTarget::ScreenFrameBuffer);
    return target ? target->framebuffer.get() : nullptr;
}

std::vector<RhiTexture*> RenderPassContext::cubeShadowMaps() const
{
    std::vector<RhiTexture*> empty;
    return m_graph && m_node ? m_graph->cubeShadowMapsOf(RenderPass::Type::Shadow) : empty;
}

RhiFrameBuffer* RenderPassContext::cubeShadowFaceFrameBufferOf(size_t cube_index, int face) const
{
    return m_graph && m_node ? m_graph->cubeShadowFaceFrameBufferOf(RenderPass::Type::Shadow, cube_index, face) : nullptr;
}

RhiFrameBuffer* RenderPassContext::cubeShadowStaticFaceFrameBufferOf(size_t cube_index, int face) const
{
    return m_graph && m_node ? m_graph->cubeShadowStaticFaceFrameBufferOf(RenderPass::Type::Shadow, cube_index, face) : nullptr;
}

void RenderPassContext::ensureCubeShadowMapsCount(size_t count)
{
    if (m_graph && m_node)
        m_graph->ensureCubeShadowMapsCount(RenderPass::Type::Shadow, count);
}
