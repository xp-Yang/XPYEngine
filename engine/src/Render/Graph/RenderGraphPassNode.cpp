#include "RenderGraphPassNode.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

RenderGraphResourceBinding RenderGraphResourceBinding::color(int attachment_index)
{
    RenderGraphResourceBinding binding;
    binding.attachment = RenderGraphResourceAttachment::Color;
    binding.color_attachment = attachment_index;
    return binding;
}

RenderGraphResourceBinding RenderGraphResourceBinding::depth()
{
    RenderGraphResourceBinding binding;
    binding.attachment = RenderGraphResourceAttachment::Depth;
    return binding;
}

RenderGraphResourceBinding RenderGraphResourceBinding::depthStencil()
{
    RenderGraphResourceBinding binding;
    binding.attachment = RenderGraphResourceAttachment::DepthStencil;
    return binding;
}

RenderGraphResourceDesc RenderGraphResourceDesc::texture(RhiTexture::Format format, int sample_count, bool transient)
{
    RenderGraphResourceDesc desc;
    desc.format = format;
    desc.sample_count = sample_count;
    desc.transient = transient;
    return desc;
}

RenderGraphPassNode::RenderGraphPassNode(std::string name, RenderPass::Type type, RenderPass* pass)
    : m_name(std::move(name))
    , m_type(type)
    , m_pass(pass)
{
}

RenderGraphPassNode& RenderGraphPassNode::read(const std::string& resource_name)
{
    if (std::find(m_reads.begin(), m_reads.end(), resource_name) == m_reads.end())
        m_reads.push_back(resource_name);
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::readAs(const std::string& slot_name, const std::string& resource_name)
{
    bindSlot(m_read_slots, slot_name, resource_name);
    return read(resource_name);
}

RenderGraphPassNode& RenderGraphPassNode::readWrite(const std::string& resource_name)
{
    if (std::find(m_read_writes.begin(), m_read_writes.end(), resource_name) == m_read_writes.end())
        m_read_writes.push_back(resource_name);
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::readWriteAs(const std::string& slot_name, const std::string& resource_name)
{
    bindSlot(m_read_write_slots, slot_name, resource_name);
    return readWrite(resource_name);
}

void RenderGraphPassNode::bindSlot(std::unordered_map<std::string, std::string>& slot_bindings, const std::string& slot_name, const std::string& resource_name)
{
    auto it = slot_bindings.find(slot_name);
    if (it == slot_bindings.end())
    {
        slot_bindings.emplace(slot_name, resource_name);
        return;
    }
    if (it->second != resource_name)
        throw std::runtime_error("RenderGraph slot is rebound to a different resource: " + slot_name);
}

void RenderGraphPassNode::declareTarget(const std::string& target_name, RenderGraphTargetKind kind, int initial_cube_map_count)
{
    auto it = std::find_if(m_targets.begin(), m_targets.end(),
        [&target_name](const TargetDeclaration& target)
        {
            return target.name == target_name;
        });
    if (it == m_targets.end())
    {
        m_targets.push_back(TargetDeclaration{ target_name, kind, initial_cube_map_count });
        return;
    }

    if (it->kind != kind)
        throw std::runtime_error("RenderGraph target is declared with conflicting kinds: " + target_name);
    if (kind == RenderGraphTargetKind::CubeDepth)
        it->initial_cube_map_count = std::max(it->initial_cube_map_count, initial_cube_map_count);
}

RenderGraphPassNode& RenderGraphPassNode::mainTarget()
{
    m_active_target = RGTarget::Main;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::target(const std::string& target_name, RenderGraphTargetKind kind)
{
    declareTarget(target_name, kind);
    m_active_target = target_name;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::backbuffer(const std::string& target_name)
{
    declareTarget(target_name, RenderGraphTargetKind::Backbuffer);
    m_active_target = target_name;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::cubeDepthTarget(const std::string& target_name, int initial_cube_map_count)
{
    declareTarget(target_name, RenderGraphTargetKind::CubeDepth, initial_cube_map_count);
    m_active_target = target_name;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::writeTo(const std::string& target_name, const std::string& resource_name, RenderGraphResourceBinding binding, RenderGraphResourceDesc desc)
{
    declareTarget(target_name, RenderGraphTargetKind::Texture);
    auto it = std::find_if(m_writes.begin(), m_writes.end(),
        [&resource_name](const ResourceWrite& write)
        {
            return write.name == resource_name;
        });
    if (it == m_writes.end())
        m_writes.push_back(ResourceWrite{ resource_name, target_name, binding, desc });
    else
    {
        it->target = target_name;
        it->binding = binding;
        it->desc = desc;
    }
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::color(const std::string& resource_name, RhiTexture::Format format, int attachment_index, int sample_count, bool transient)
{
    return writeTo(m_active_target, resource_name, RenderGraphResourceBinding::color(attachment_index), RenderGraphResourceDesc::texture(format, sample_count, transient));
}

RenderGraphPassNode& RenderGraphPassNode::depth(const std::string& resource_name, RhiTexture::Format format, int sample_count, bool transient)
{
    return writeTo(m_active_target, resource_name, RenderGraphResourceBinding::depth(), RenderGraphResourceDesc::texture(format, sample_count, transient));
}

RenderGraphPassNode& RenderGraphPassNode::depthStencil(const std::string& resource_name, RhiTexture::Format format, int sample_count, bool transient)
{
    return writeTo(m_active_target, resource_name, RenderGraphResourceBinding::depthStencil(), RenderGraphResourceDesc::texture(format, sample_count, transient));
}

RenderGraphPassNode& RenderGraphPassNode::setEnabled(bool enabled)
{
    m_enabled = enabled;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::setDisabledExecution(RenderGraphDisabledExecution execution)
{
    m_disabled_execution = execution;
    return *this;
}

RenderGraphPassNode& RenderGraphPassNode::setSetup(std::function<void(RenderPass&)> setup)
{
    m_setup = std::move(setup);
    return *this;
}
