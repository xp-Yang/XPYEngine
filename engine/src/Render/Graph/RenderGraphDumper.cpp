#include "RenderGraphDumper.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace {

const char* passTypeName(RenderPass::Type type)
{
    switch (type)
    {
    case RenderPass::Type::ZPre:
        return "ZPre";
    case RenderPass::Type::Picking:
        return "Picking";
    case RenderPass::Type::SkyBox:
        return "SkyBox";
    case RenderPass::Type::Shadow:
        return "Shadow";
    case RenderPass::Type::Forward:
        return "Forward";
    case RenderPass::Type::GBuffer:
        return "GBuffer";
    case RenderPass::Type::DeferredLighting:
        return "DeferredLighting";
    case RenderPass::Type::Transparent:
        return "Transparent";
    case RenderPass::Type::Bloom:
        return "Bloom";
    case RenderPass::Type::Outline:
        return "Outline";
    case RenderPass::Type::Combined:
        return "Combined";
    case RenderPass::Type::WireFrame:
        return "WireFrame";
    case RenderPass::Type::CheckerBoard:
        return "CheckerBoard";
    case RenderPass::Type::Normal:
        return "Normal";
    case RenderPass::Type::RayTracing:
        return "RayTracing";
    default:
        return "Unknown";
    }
}

const char* attachmentName(RenderGraph::ResourceAttachment attachment)
{
    switch (attachment)
    {
    case RenderGraph::ResourceAttachment::Color:
        return "color";
    case RenderGraph::ResourceAttachment::Depth:
        return "depth";
    case RenderGraph::ResourceAttachment::DepthStencil:
        return "depth-stencil";
    default:
        return "unknown";
    }
}

const char* targetKindName(RenderGraph::RenderTargetKind kind)
{
    switch (kind)
    {
    case RenderGraph::RenderTargetKind::Texture:
        return "texture";
    case RenderGraph::RenderTargetKind::Backbuffer:
        return "backbuffer";
    case RenderGraph::RenderTargetKind::CubeDepth:
        return "cube-depth";
    default:
        return "unknown";
    }
}

std::string bindingName(const RenderGraph::ResourceBinding& binding)
{
    std::ostringstream stream;
    stream << attachmentName(binding.attachment);
    if (binding.attachment == RenderGraph::ResourceAttachment::Color)
        stream << binding.color_attachment;
    return stream.str();
}

const char* formatName(RhiTexture::Format format)
{
    switch (format)
    {
    case RhiTexture::Format::UnknownFormat:
        return "Unknown";
    case RhiTexture::Format::R8:
        return "R8";
    case RhiTexture::Format::RGB8:
        return "RGB8";
    case RhiTexture::Format::RGB16F:
        return "RGB16F";
    case RhiTexture::Format::RGBA8:
        return "RGBA8";
    case RhiTexture::Format::RGBA16F:
        return "RGBA16F";
    case RhiTexture::Format::DEPTH24STENCIL8:
        return "DEPTH24STENCIL8";
    case RhiTexture::Format::DEPTH:
        return "DEPTH";
    default:
        return "Unknown";
    }
}

std::string descName(const RenderGraph::ResourceDesc& desc)
{
    std::ostringstream stream;
    stream << formatName(desc.format) << " x" << desc.sample_count;
    if (desc.transient)
        stream << " transient";
    return stream.str();
}

std::string joinPassTypes(const std::vector<RenderPass::Type>& values)
{
    std::ostringstream stream;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            stream << ", ";
        stream << passTypeName(values[i]);
    }
    return stream.str();
}

std::string joinStrings(const std::vector<std::string>& values)
{
    std::ostringstream stream;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            stream << ", ";
        stream << values[i];
    }
    return stream.str();
}

std::string joinSlots(const std::unordered_map<std::string, std::string>& values)
{
    std::vector<std::string> names;
    names.reserve(values.size());
    for (const auto& value : values)
        names.push_back(value.first);
    std::sort(names.begin(), names.end());

    std::ostringstream stream;
    for (size_t i = 0; i < names.size(); ++i)
    {
        if (i > 0)
            stream << ", ";
        stream << names[i] << "=" << values.at(names[i]);
    }
    return stream.str();
}

std::string joinTargets(const std::vector<RenderGraph::PassNode::TargetDeclaration>& values)
{
    std::ostringstream stream;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            stream << ", ";
        stream << values[i].name << " (" << targetKindName(values[i].kind) << ")";
    }
    return stream.str();
}

std::string joinWrites(const std::vector<RenderGraph::PassNode::ResourceWrite>& values)
{
    std::ostringstream stream;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            stream << ", ";
        stream << values[i].name << " -> " << values[i].target
               << " (" << bindingName(values[i].binding) << ", " << descName(values[i].desc) << ")";
    }
    return stream.str();
}

} // namespace

RenderGraphDumper::RenderGraphDumper(const RenderGraph& graph)
    : m_graph(graph)
{
}

std::vector<std::string> RenderGraphDumper::resourceNames() const
{
    std::unordered_set<RenderPass::Type> compiled_passes;
    compiled_passes.reserve(m_graph.m_compiled_order.size());
    for (RenderPass::Type type : m_graph.m_compiled_order)
        compiled_passes.insert(type);

    std::vector<std::string> names;
    names.reserve(m_graph.m_resources.size());
    for (const auto& resource : m_graph.m_resources)
    {
        const RenderGraph::ResourceState& state = resource.second;
        if (compiled_passes.find(state.owner_pass) != compiled_passes.end() ||
            compiled_passes.find(state.last_modifier) != compiled_passes.end())
            names.push_back(resource.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::string RenderGraphDumper::graph() const
{
    std::ostringstream stream;
    stream << "RenderGraph\n";
    stream << "Passes:\n";

    for (const RenderGraph::PassNode& node : m_graph.m_nodes)
    {
        stream << "- " << node.m_name << " [" << passTypeName(node.m_type) << "] "
               << (node.m_enabled ? "enabled" : "disabled") << "\n";
        if (!node.m_reads.empty())
            stream << "  reads: " << joinStrings(node.m_reads) << "\n";
        if (!node.m_read_slots.empty())
            stream << "  read slots: " << joinSlots(node.m_read_slots) << "\n";
        if (!node.m_read_writes.empty())
            stream << "  read-writes: " << joinStrings(node.m_read_writes) << "\n";
        if (!node.m_read_write_slots.empty())
            stream << "  read-write slots: " << joinSlots(node.m_read_write_slots) << "\n";
        if (!node.m_targets.empty())
            stream << "  targets: " << joinTargets(node.m_targets) << "\n";
        if (!node.m_writes.empty())
            stream << "  writes: " << joinWrites(node.m_writes) << "\n";
        if (!node.m_resolved_dependencies.empty())
            stream << "  resolved dependencies: " << joinPassTypes(node.m_resolved_dependencies) << "\n";
    }

    std::vector<std::string> names = resourceNames();
    if (!names.empty())
    {
        stream << "Resources:\n";
        for (const std::string& resource_name : names)
        {
            const RenderGraph::ResourceState& state = m_graph.m_resources.at(resource_name);
            stream << "- " << resource_name
                   << " owner=" << passTypeName(state.owner_pass)
                   << " last_modifier=" << passTypeName(state.last_modifier)
                   << " target=" << state.target
                   << " binding=" << bindingName(state.binding)
                   << " desc=" << descName(state.desc) << "\n";
        }
    }

    if (!m_graph.m_resource_outputs.empty())
        stream << "Resource outputs: " << joinStrings(m_graph.m_resource_outputs) << "\n";

    return stream.str();
}

std::string RenderGraphDumper::executionOrder() const
{
    std::ostringstream stream;
    if (m_graph.m_compiled_order.empty())
    {
        stream << "<empty>\n";
        return stream.str();
    }

    for (size_t i = 0; i < m_graph.m_compiled_order.size(); ++i)
    {
        const RenderGraph::PassNode* node = m_graph.findNode(m_graph.m_compiled_order[i]);
        stream << i << ": ";
        if (node)
            stream << node->m_name << " [" << passTypeName(node->m_type) << "]";
        else
            stream << passTypeName(m_graph.m_compiled_order[i]);
        stream << "\n";
    }
    return stream.str();
}
