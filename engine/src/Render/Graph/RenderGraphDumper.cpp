#include "RenderGraphDumper.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_map>
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
    case RenderPass::Type::FXAA:
        return "FXAA";
    case RenderPass::Type::ColorGrading:
        return "ColorGrading";
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
    case RenderPass::Type::UI:
        return "UI";
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

bool isDepthAttachment(RhiAttachment::Type attachment)
{
    return attachment == RhiAttachment::Type::Depth || attachment == RhiAttachment::Type::DepthStencil;
}

void appendUnique(std::vector<RenderPass::Type>& values, RenderPass::Type value)
{
    if (std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(value);
}

void appendUnique(std::vector<std::string>& values, const std::string& value)
{
    if (std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(value);
}

void appendPassNames(std::vector<std::string>& out, const std::vector<RenderPass::Type>& values)
{
    for (RenderPass::Type value : values)
        appendUnique(out, passTypeName(value));
}

void appendPasses(std::vector<RenderPass::Type>& out, const std::vector<RenderPass::Type>& values)
{
    for (RenderPass::Type value : values)
        appendUnique(out, value);
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

std::string joinWrites(const std::vector<RenderGraphResource>& values)
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
    std::vector<std::string> names;
    for (const RenderGraphResourceDebugInfo& info : resourceInfos())
        names.push_back(info.name);
    return names;
}

std::vector<RenderGraphResourceDebugInfo> RenderGraphDumper::resourceInfos() const
{
    std::unordered_set<RenderPass::Type> compiled_passes;
    compiled_passes.reserve(m_graph.m_ordered_nodes.size());
    std::map<RenderPass::Type, size_t> pass_order;
    for (size_t i = 0; i < m_graph.m_ordered_nodes.size(); ++i)
    {
        RenderGraphPassNode* node = m_graph.m_ordered_nodes[i];
        if (node)
        {
            compiled_passes.insert(node->m_type);
            pass_order[node->m_type] = i;
        }
    }

    std::vector<RGResourceName> ordered_names;
    ordered_names.reserve(m_graph.m_resources.size());
    std::unordered_set<RGResourceName> emitted_names;

    auto appendResourceIfCurrent = [&](const RGResourceName& resource_name, RenderPass::Type pass)
    {
        auto it = m_graph.m_resources.find(resource_name);
        if (it == m_graph.m_resources.end() || it->second.last_modifier_pass != pass)
            return;
        if (emitted_names.insert(resource_name).second)
            ordered_names.push_back(resource_name);
    };

    for (RenderGraphPassNode* node : m_graph.m_ordered_nodes)
    {
        if (!node)
            continue;
        for (const RGResourceName& resource_name : node->m_modifies)
            appendResourceIfCurrent(resource_name, node->m_type);
        for (const RenderGraphResource& write : node->m_resources)
            appendResourceIfCurrent(write.name, node->m_type);
    }

    std::vector<RGResourceName> remaining_names;
    for (const auto& resource_pair : m_graph.m_resources)
    {
        const RenderGraphResource& resource = resource_pair.second;
        if (emitted_names.find(resource_pair.first) != emitted_names.end())
            continue;
        if (compiled_passes.find(resource.owner_pass) != compiled_passes.end() ||
            compiled_passes.find(resource.last_modifier_pass) != compiled_passes.end())
            remaining_names.push_back(resource_pair.first);
    }
    std::sort(remaining_names.begin(), remaining_names.end());
    for (const RGResourceName& resource_name : remaining_names)
        ordered_names.push_back(resource_name);

    std::unordered_map<RGResourceName, std::vector<RenderPass::Type>> direct_history;
    std::unordered_map<RGResourceName, std::vector<RenderPass::Type>> contributors;
    for (RenderGraphPassNode* node : m_graph.m_ordered_nodes)
    {
        if (!node)
            continue;

        const bool clear_writes = !node->m_enabled && node->m_disabled_execution == RGDisabledExecution::Clear;
        if (!node->m_enabled && !clear_writes)
            continue;

        std::vector<RenderPass::Type> input_contributors;
        if (node->m_enabled)
        {
            for (const RGResourceName& resource_name : node->m_reads)
                appendPasses(input_contributors, contributors[resource_name]);
            for (const RGResourceName& resource_name : node->m_modifies)
                appendPasses(input_contributors, contributors[resource_name]);
        }

        for (const RenderGraphResource& write : node->m_resources)
        {
            direct_history[write.name].clear();
            contributors[write.name].clear();
            appendUnique(direct_history[write.name], node->m_type);
            appendPasses(contributors[write.name], input_contributors);
            appendUnique(contributors[write.name], node->m_type);
        }

        if (!node->m_enabled)
            continue;

        for (const RGResourceName& resource_name : node->m_modifies)
        {
            appendUnique(direct_history[resource_name], node->m_type);
            appendPasses(contributors[resource_name], input_contributors);
            appendUnique(contributors[resource_name], node->m_type);
        }
    }

    std::vector<RenderGraphResourceDebugInfo> infos;
    infos.reserve(ordered_names.size());
    auto sortByExecutionOrder = [&pass_order](std::vector<RenderPass::Type>& values)
    {
        std::stable_sort(values.begin(), values.end(),
            [&pass_order](RenderPass::Type lhs, RenderPass::Type rhs)
            {
                const auto lhs_it = pass_order.find(lhs);
                const auto rhs_it = pass_order.find(rhs);
                const size_t lhs_order = lhs_it == pass_order.end() ? static_cast<size_t>(-1) : lhs_it->second;
                const size_t rhs_order = rhs_it == pass_order.end() ? static_cast<size_t>(-1) : rhs_it->second;
                return lhs_order < rhs_order;
            });
    };

    for (const RGResourceName& resource_name : ordered_names)
    {
        auto it = m_graph.m_resources.find(resource_name);
        if (it == m_graph.m_resources.end())
            continue;

        const RenderGraphResource& resource = it->second;
        const RhiAttachmentDesc& desc = resource.attachment_desc;
        RhiTexture* texture = m_graph.textureOf(resource_name);

        RenderGraphResourceDebugInfo info;
        info.name = resource_name;
        info.owner_pass = passTypeName(resource.owner_pass);
        info.last_modifier_pass = passTypeName(resource.last_modifier_pass);
        info.render_target = resource.owner_target_name;
        info.attachment = bindingName(desc);
        info.format = formatName(desc.format);
        info.color_attachment_index = desc.color_attachment_index;
        info.sample_count = desc.sample_count;
        info.transient = desc.transient;
        info.is_depth = isDepthAttachment(desc.attachment_type);
        info.size = texture ? texture->pixelSize() : Vec2{};
        info.texture_id = texture ? texture->id() : 0;
        std::vector<RenderPass::Type> resource_history = direct_history[resource_name];
        std::vector<RenderPass::Type> resource_contributors = contributors[resource_name];
        sortByExecutionOrder(resource_history);
        sortByExecutionOrder(resource_contributors);
        appendPassNames(info.direct_history, resource_history);
        appendPassNames(info.contributors, resource_contributors);

        infos.push_back(info);
    }
    return infos;
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
        if (!node.m_modifies.empty())
            stream << "  read-writes: " << joinStrings(node.m_modifies) << "\n";
        if (!node.m_targets.empty())
            stream << "  targets: " << joinTargets(node.m_targets) << "\n";
        if (!node.m_resources.empty())
            stream << "  writes: " << joinWrites(node.m_resources) << "\n";
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
                   << " target=" << resource.owner_target_name
                   << " binding=" << bindingName(resource.attachment_desc)
                   << " desc=" << descName(resource.attachment_desc) << "\n";
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
