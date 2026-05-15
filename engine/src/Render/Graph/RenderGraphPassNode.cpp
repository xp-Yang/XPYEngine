#include "RenderGraphPassNode.hpp"

RenderGraphPassNode::RenderGraphPassNode(RenderPass::Type type, RenderPass* pass)
    : m_type(type)
    , m_pass(pass)
{
}

RenderGraphPassNode& RenderGraphPassNode::read(const RGResourceName& resource_name)
{
    if (std::find(m_reads.begin(), m_reads.end(), resource_name) == m_reads.end())
        m_reads.push_back(resource_name);
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::readWrite(const RGResourceName& resource_name)
{
    if (std::find(m_read_writes.begin(), m_read_writes.end(), resource_name) == m_read_writes.end())
        m_read_writes.push_back(resource_name);
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::target(const RGTargetName& target_name, RenderTargetType type, int initial_cube_map_count)
{
    auto it = std::find_if(m_targets.begin(), m_targets.end(),
        [&target_name](const RenderTargetDeclaration& target)
        {
            return target.name == target_name;
        });

    if (it == m_targets.end())
    {
        m_targets.push_back(RenderTargetDeclaration{ target_name, type, initial_cube_map_count });
    }
    else
    {
        if (it->render_target_type != type)
            throw std::runtime_error("RenderGraph target is declared with conflicting types: " + target_name);
        if (type == RenderTargetType::CubeDepth)
            it->initial_cube_map_count = std::max(it->initial_cube_map_count, initial_cube_map_count);
    }
    m_active_target = target_name;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::color(const RGResourceName& resource_name, RhiTexture::Format format, int attachment_index, int sample_count, bool transient)
{
    return writeTo(resource_name, RhiAttachmentDesc{ RhiAttachment::Type::Color, attachment_index, format, sample_count, transient });
}

RenderGraphPassNode& RenderGraphPassNode::depth(const RGResourceName& resource_name, RhiTexture::Format format, int sample_count, bool transient)
{
    return writeTo(resource_name, RhiAttachmentDesc{ RhiAttachment::Type::Depth, 0, format, sample_count, transient });
}

RenderGraphPassNode& RenderGraphPassNode::depthStencil(const RGResourceName& resource_name, RhiTexture::Format format, int sample_count, bool transient)
{
    return writeTo(resource_name, RhiAttachmentDesc{ RhiAttachment::Type::DepthStencil, 0, format, sample_count, transient });
}

RenderGraphPassNode& RenderGraphPassNode::setEnabled(bool enabled)
{
    m_enabled = enabled;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::setDisabledExecution(RGDisabledExecution execution)
{
    m_disabled_execution = execution;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::setSetup(std::function<void(RenderPass&)> setup)
{
    m_setup = std::move(setup);
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::writeTo(const RGResourceName& resource_name, const RhiAttachmentDesc& desc)
{
    auto it = std::find_if(m_writes.begin(), m_writes.end(),
        [&resource_name](const ResourceDeclaration& write)
        {
            return write.name == resource_name;
        });
    if (it == m_writes.end())
        m_writes.push_back(ResourceDeclaration{ resource_name, m_active_target, desc });
    else
    {
        it->owner_target_name = m_active_target;
        it->attachment_desc = desc;
    }
    return *this;
}

void RenderGraphPassNode::addResolvedDependency(RenderPass::Type type)
{
    if (type == m_type)
        return;
    if (std::find(m_resolved_dependencies.begin(), m_resolved_dependencies.end(), type) == m_resolved_dependencies.end())
        m_resolved_dependencies.push_back(type);
}
