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

RenderGraphPassNode& RenderGraphPassNode::modify(const RGResourceName& resource_name)
{
    if (std::find(m_modifies.begin(), m_modifies.end(), resource_name) == m_modifies.end())
        m_modifies.push_back(resource_name);
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::target(const RGTargetName& target_name, RenderTargetType type)
{
    auto it = std::find_if(m_targets.begin(), m_targets.end(),
        [&target_name](const RenderTargetDeclaration& target)
        {
            return target.name == target_name;
        });

    if (it == m_targets.end())
    {
        m_targets.push_back(RenderTargetDeclaration{ target_name, m_type, type });
    }
    else
    {
        if (it->render_target_type != type)
            throw std::runtime_error("RenderGraph target is declared with conflicting types: " + target_name);
    }
    m_active_target = target_name;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::color(const RGResourceName& resource_name, RhiTexture::Format format, int attachment_index, int sample_count, bool transient)
{
    return produce(resource_name, RhiAttachmentDesc{ RhiAttachment::Type::Color, attachment_index, format, sample_count, transient });
}

RenderGraphPassNode& RenderGraphPassNode::depth(const RGResourceName& resource_name, RhiTexture::Format format, int sample_count, bool transient)
{
    return produce(resource_name, RhiAttachmentDesc{ RhiAttachment::Type::Depth, 0, format, sample_count, transient });
}

RenderGraphPassNode& RenderGraphPassNode::depthStencil(const RGResourceName& resource_name, RhiTexture::Format format, int sample_count, bool transient)
{
    return produce(resource_name, RhiAttachmentDesc{ RhiAttachment::Type::DepthStencil, 0, format, sample_count, transient });
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

RenderGraphPassNode& RenderGraphPassNode::produce(const RGResourceName& resource_name, const RhiAttachmentDesc& desc)
{
    auto it_resource = std::find_if(m_resources.begin(), m_resources.end(),
        [&resource_name](const RenderGraphResource& resource)
        {
            return resource.name == resource_name;
        });
    if (it_resource == m_resources.end())
        m_resources.push_back(RenderGraphResource{ resource_name, m_type, m_type, m_active_target, desc });
    else
    {
        throw std::runtime_error("RenderGraph resource is declared multiple times by one pass: " + resource_name);
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
