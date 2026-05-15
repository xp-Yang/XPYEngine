#include "RenderPassContext.hpp"

RenderPassContext::RenderPassContext(RenderGraph& graph, const RenderGraph::PassNode& node)
    : m_graph(&graph)
    , m_node(&node)
{
}

RhiTexture* RenderPassContext::texture(const std::string& resource_name) const
{
    return m_graph ? m_graph->texture(resource_name) : nullptr;
}

RhiFrameBuffer* RenderPassContext::frameBuffer(const std::string& resource_name) const
{
    return m_graph ? m_graph->frameBuffer(resource_name) : nullptr;
}

RhiFrameBuffer* RenderPassContext::targetFrameBuffer(const std::string& target_name) const
{
    return m_graph && m_node ? m_graph->targetFrameBuffer(m_node->m_type, target_name) : nullptr;
}

RhiFrameBuffer* RenderPassContext::defaultFrameBuffer() const
{
    return targetFrameBuffer(RGTarget::Backbuffer);
}

const std::vector<unsigned int>& RenderPassContext::cubeDepthTextures(const std::string& target_name) const
{
    static const std::vector<unsigned int> empty;
    return m_graph && m_node ? m_graph->cubeDepthTextures(m_node->m_type, target_name) : empty;
}

unsigned int RenderPassContext::cubeDepthFrameBuffer(const std::string& target_name) const
{
    const RenderGraph::RenderTargetState* state = m_graph && m_node ? m_graph->targetState(m_node->m_type, target_name) : nullptr;
    return state && state->kind == RenderGraph::RenderTargetKind::CubeDepth ? state->cube_framebuffer : 0;
}

int RenderPassContext::cubeDepthEdge(const std::string& target_name) const
{
    const RenderGraph::RenderTargetState* state = m_graph && m_node ? m_graph->targetState(m_node->m_type, target_name) : nullptr;
    return state && state->kind == RenderGraph::RenderTargetKind::CubeDepth ? state->cube_edge : 0;
}

void RenderPassContext::ensureCubeDepthTextureCount(const std::string& target_name, size_t count)
{
    if (m_graph && m_node)
        m_graph->ensureCubeDepthTargetCapacity(m_node->m_type, target_name, count);
}

RhiFrameBuffer* RenderPassContext::readFrameBuffer(const std::string& slot_name) const
{
    const std::string* resource_name = m_node ? resourceNameForSlot(m_node->m_read_slots, slot_name) : nullptr;
    return resource_name && m_graph ? m_graph->frameBuffer(*resource_name) : nullptr;
}

RhiTexture* RenderPassContext::readTexture(const std::string& slot_name) const
{
    const std::string* resource_name = m_node ? resourceNameForSlot(m_node->m_read_slots, slot_name) : nullptr;
    return resource_name ? texture(*resource_name) : nullptr;
}

RhiFrameBuffer* RenderPassContext::readWriteFrameBuffer(const std::string& slot_name) const
{
    const std::string* resource_name = m_node ? resourceNameForSlot(m_node->m_read_write_slots, slot_name) : nullptr;
    return resource_name && m_graph ? m_graph->frameBuffer(*resource_name) : nullptr;
}

const std::string* RenderPassContext::resourceNameForSlot(const std::unordered_map<std::string, std::string>& slot_bindings, const std::string& slot_name) const
{
    auto it = slot_bindings.find(slot_name);
    return it == slot_bindings.end() ? nullptr : &it->second;
}
