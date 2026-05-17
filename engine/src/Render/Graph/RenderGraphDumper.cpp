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
    case RenderPass::Type::WireFrame:
        return "WireFrame";
    case RenderPass::Type::CheckerBoard:
        return "CheckerBoard";
    case RenderPass::Type::Normal:
        return "Normal";
    case RenderPass::Type::RayTracing:
        return "RayTracing";
    case RenderPass::Type::Final:
        return "Final";
    default:
        return "Unknown";
    }
}

const char* attachmentName(RhiAttachment::Type attachment)
{
    switch (attachment)
    {
    case RhiAttachment::Type::Color:
        return "color";
    case RhiAttachment::Type::Depth:
        return "depth";
    case RhiAttachment::Type::DepthStencil:
        return "depth-stencil";
    default:
        return "unknown";
    }
}

const char* targetKindName(RenderTargetType kind)
{
    switch (kind)
    {
    case RenderTargetType::FrameBuffer:
        return "framebuffer";
    case RenderTargetType::ScreenFrameBuffer:
        return "screen-framebuffer";
    case RenderTargetType::CubeDepth:
        return "cube-depth";
    default:
        return "unknown";
    }
}

std::string bindingName(const RhiAttachmentDesc& desc)
{
    std::ostringstream stream;
    stream << attachmentName(desc.attachment_type);
    if (desc.attachment_type == RhiAttachment::Type::Color)
        stream << desc.color_attachment_index;
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

std::string descName(const RhiAttachmentDesc& desc)
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

std::string joinTargets(const std::vector<RenderTargetDeclaration>& values)
{
    std::ostringstream stream;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            stream << ", ";
        stream << values[i].name << " (" << targetKindName(values[i].render_target_type) << ")";
    }
    return stream.str();
}

std::string joinWrites(const std::vector<ResourceDeclaration>& values)
{
    std::ostringstream stream;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            stream << ", ";
        stream << values[i].name << " -> " << values[i].owner_target_name
               << " (" << bindingName(values[i].attachment_desc) << ", " << descName(values[i].attachment_desc) << ")";
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
    compiled_passes.reserve(m_graph.m_ordered_nodes.size());
    for (RenderGraphPassNode* node : m_graph.m_ordered_nodes)
        compiled_passes.insert(node->m_type);

    std::vector<std::string> names;
    names.reserve(m_graph.m_resources.size());
    for (const auto& resource_pair : m_graph.m_resources)
    {
        const RenderGraphResource& resource = resource_pair.second;
        if (compiled_passes.find(resource.owner_pass) != compiled_passes.end() ||
            compiled_passes.find(resource.last_modifier_pass) != compiled_passes.end())
            names.push_back(resource_pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::string RenderGraphDumper::graph() const
{
    std::ostringstream stream;
    stream << "RenderGraph\n";
    stream << "Passes:\n";

    for (const RenderGraphPassNode& node : m_graph.m_nodes)
    {
        stream << "- " << " [" << passTypeName(node.m_type) << "] "
               << (node.m_enabled ? "enabled" : "disabled") << "\n";
        if (!node.m_reads.empty())
            stream << "  reads: " << joinStrings(node.m_reads) << "\n";
        if (!node.m_read_writes.empty())
            stream << "  read-writes: " << joinStrings(node.m_read_writes) << "\n";
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
            const RenderGraphResource& resource = m_graph.m_resources.at(resource_name);
            stream << "- " << resource_name
                   << " owner=" << passTypeName(resource.owner_pass)
                   << " last_modifier_pass=" << passTypeName(resource.last_modifier_pass)
                   << " target=" << resource.resource_decl.owner_target_name
                   << " binding=" << bindingName(resource.resource_decl.attachment_desc)
                   << " desc=" << descName(resource.resource_decl.attachment_desc) << "\n";
        }
    }

    if (!m_graph.m_outputs.empty())
        stream << "Resource outputs: " << joinStrings(m_graph.m_outputs) << "\n";

    return stream.str();
}

std::string RenderGraphDumper::executionOrder() const
{
    std::ostringstream stream;
    if (m_graph.m_ordered_nodes.empty())
    {
        stream << "<empty>\n";
        return stream.str();
    }

    for (size_t i = 0; i < m_graph.m_ordered_nodes.size(); ++i)
    {
        const RenderGraphPassNode* node = m_graph.m_ordered_nodes[i];
        stream << i << ": ";
        if (node)
            stream << " [" << passTypeName(node->m_type) << "]";
        else
            stream << passTypeName(m_graph.m_ordered_nodes[i]->m_type);
        stream << "\n";
    }
    return stream.str();
}
