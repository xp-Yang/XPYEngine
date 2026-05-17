#include "RenderPassContext.hpp"

RenderPassContext::RenderPassContext(RenderGraph& graph, const RenderGraphPassNode& node, RenderSourceData& render_source_data)
    : m_graph(&graph)
    , m_node(&node)
    , m_render_source_data(&render_source_data)
{
}

RenderSourceData& RenderPassContext::renderSourceData() const
{
    return *m_render_source_data;
}

RhiTexture* RenderPassContext::texture(const std::string& resource_name) const
{
    return m_graph ? m_graph->textureOf(resource_name) : nullptr;
}

RhiFrameBuffer* RenderPassContext::frameBuffer(const std::string& resource_name) const
{
    return m_graph ? m_graph->frameBufferOf(resource_name) : nullptr;
}

RhiFrameBuffer* RenderPassContext::targetFrameBuffer(const std::string& target_name) const
{
    const RenderGraphRenderTarget* target = m_graph->findRenderTarget(m_node->m_type, target_name);
    return target ? target->framebuffer.get() : nullptr;
}

RhiFrameBuffer* RenderPassContext::defaultFrameBuffer() const
{
    const RenderGraphRenderTarget* target = m_graph->findRenderTarget(m_node->m_type, RGTarget::ScreenFrameBuffer);
    return target ? target->framebuffer.get() : nullptr;
}

const std::vector<unsigned int>& RenderPassContext::cubeDepthTextures(const std::string& target_name) const
{
    static const std::vector<unsigned int> empty;
    return m_graph && m_node ? m_graph->cubeDepthTextures(m_node->m_type) : empty;
}

unsigned int RenderPassContext::cubeDepthFrameBuffer(const std::string& target_name) const
{
    const RenderGraphRenderTarget* state = m_graph && m_node ? m_graph->findRenderTarget(m_node->m_type, target_name) : nullptr;
    return state ? state->cube_framebuffer : 0;
}

int RenderPassContext::cubeDepthEdge(const std::string& target_name) const
{
    const RenderGraphRenderTarget* state = m_graph && m_node ? m_graph->findRenderTarget(m_node->m_type, target_name) : nullptr;
    return state ? state->cube_edge : 0;
}

void RenderPassContext::ensureCubeDepthTextureCount(const std::string& target_name, size_t count)
{
    if (m_graph && m_node)
        m_graph->ensureCubeDepthTargetCapacity(m_node->m_type, count);
}
