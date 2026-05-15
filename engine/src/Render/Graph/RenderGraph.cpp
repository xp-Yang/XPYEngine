#include "RenderGraph.hpp"
#include "RenderPassContext.hpp"

#include <algorithm>
#include <cassert>
#include <glad/glad.h>
#include <stdexcept>

namespace {

bool samePixelSize(const Vec2& lhs, const Vec2& rhs)
{
    return (int)lhs.x == (int)rhs.x && (int)lhs.y == (int)rhs.y;
}

} // namespace

RenderGraph::~RenderGraph()
{
    for (auto& target : m_targets)
        destroyCubeDepthTarget(target.second);
}

void RenderGraph::reset()
{
    m_nodes.clear();
    m_node_indices.clear();
    m_resource_outputs.clear();
    m_resources.clear();
    m_compiled_order.clear();
}

void RenderGraph::setFrameSize(const Vec2& pixel_size)
{
    m_frame_size = pixel_size;
}

RenderGraph::PassNode& RenderGraph::addPass(const std::string& name, RenderPass::Type type, RenderPass* pass)
{
    assert(pass);
    assert(m_node_indices.find(type) == m_node_indices.end());

    const size_t index = m_nodes.size();
    m_nodes.emplace_back(name, type, pass);
    m_node_indices[type] = index;
    return m_nodes.back();
}

void RenderGraph::markOutput(const std::string& resource_name)
{
    if (std::find(m_resource_outputs.begin(), m_resource_outputs.end(), resource_name) == m_resource_outputs.end())
        m_resource_outputs.push_back(resource_name);
}

void RenderGraph::compile()
{
    m_compiled_order.clear();
    resolveResourceDependencies();
    ensureTargets();

    std::unordered_set<RenderPass::Type> visiting;
    std::unordered_set<RenderPass::Type> visited;

    if (!m_resource_outputs.empty())
    {
        for (const std::string& resource_name : m_resource_outputs)
        {
            auto it = m_resources.find(resource_name);
            if (it == m_resources.end())
                throw std::runtime_error("RenderGraph output resource is not written: " + resource_name);
            visit(it->second.last_modifier, visiting, visited);
        }
    }

    if (m_resource_outputs.empty())
    {
        for (const auto& node : m_nodes)
            visit(node.m_type, visiting, visited);
        return;
    }
}

void RenderGraph::execute(RenderSourceData& render_source_data)
{
    for (RenderPass::Type type : m_compiled_order)
    {
        PassNode* node = findNode(type);
        if (!node || !node->m_pass)
            continue;

        if (!node->m_enabled)
        {
            if (node->m_disabled_execution == DisabledExecution::Clear)
                clearPassTargets(*node);
            continue;
        }

        if (node->m_setup)
            node->m_setup(*node->m_pass);

        RenderPassContext context(*this, *node, render_source_data);
        node->m_pass->draw(context);
    }
}

bool RenderGraph::sameResourceDesc(const ResourceDesc& lhs, const ResourceDesc& rhs)
{
    return lhs.format == rhs.format &&
           lhs.sample_count == rhs.sample_count &&
           lhs.transient == rhs.transient;
}

bool RenderGraph::sameFrameBufferDesc(const FrameBufferDesc& lhs, const FrameBufferDesc& rhs)
{
    if (lhs.has_color != rhs.has_color ||
        lhs.has_depth != rhs.has_depth ||
        lhs.has_depth_stencil != rhs.has_depth_stencil)
        return false;

    for (size_t i = 0; i < lhs.colors.size(); ++i)
    {
        if (lhs.has_color[i] && !sameResourceDesc(lhs.colors[i], rhs.colors[i]))
            return false;
    }

    if (lhs.has_depth && !sameResourceDesc(lhs.depth, rhs.depth))
        return false;
    if (lhs.has_depth_stencil && !sameResourceDesc(lhs.depth_stencil, rhs.depth_stencil))
        return false;
    return true;
}

bool RenderGraph::isFrameBufferDescEmpty(const FrameBufferDesc& desc)
{
    if (desc.has_depth || desc.has_depth_stencil)
        return false;
    return std::none_of(desc.has_color.begin(), desc.has_color.end(), [](bool has_color) { return has_color; });
}

const RenderGraph::RenderTargetState* RenderGraph::targetState(RenderPass::Type type, const std::string& target_name) const
{
    auto it = m_targets.find(TargetKey{ type, target_name });
    return it == m_targets.end() ? nullptr : &it->second;
}

RenderGraph::RenderTargetState* RenderGraph::targetState(RenderPass::Type type, const std::string& target_name)
{
    auto it = m_targets.find(TargetKey{ type, target_name });
    return it == m_targets.end() ? nullptr : &it->second;
}

RhiFrameBuffer* RenderGraph::targetFrameBuffer(RenderPass::Type type, const std::string& target_name) const
{
    const RenderTargetState* state = targetState(type, target_name);
    return state ? state->framebuffer.get() : nullptr;
}

const std::vector<unsigned int>& RenderGraph::cubeDepthTextures(RenderPass::Type type, const std::string& target_name) const
{
    static const std::vector<unsigned int> empty;
    const RenderTargetState* state = targetState(type, target_name);
    if (!state || state->kind != RenderTargetKind::CubeDepth)
        return empty;
    return state->cube_maps;
}

void RenderGraph::ensureTargets()
{
    for (PassNode& node : m_nodes)
    {
        for (const PassNode::TargetDeclaration& target : node.m_targets)
        {
            TargetKey key{ node.m_type, target.name };
            RenderTargetState& state = m_targets[key];
            if (state.kind != target.kind)
            {
                if (state.framebuffer)
                    state.framebuffer->destroyGPU();
                state.framebuffer.reset();
                destroyCubeDepthTarget(state);
            }
            state.kind = target.kind;
            state.initial_cube_map_count = std::max(state.initial_cube_map_count, target.initial_cube_map_count);
        }
    }

    for (PassNode& node : m_nodes)
    {
        for (const PassNode::TargetDeclaration& target : node.m_targets)
        {
            TargetKey key{ node.m_type, target.name };
            RenderTargetState& target_state = m_targets[key];
            FrameBufferDesc desc;

            if (target.kind == RenderTargetKind::Texture)
            {
                for (const auto& resource : m_resources)
                {
                    const ResourceState& state = resource.second;
                    if (state.owner_pass != node.m_type || state.target != target.name)
                        continue;

                    switch (state.binding.attachment)
                    {
                    case ResourceAttachment::Color:
                    {
                        const int index = state.binding.color_attachment;
                        if (index < 0 || index >= (int)desc.has_color.size())
                            throw std::runtime_error("RenderGraph color attachment index is out of range: " + resource.first);
                        if (desc.has_color[index])
                            throw std::runtime_error("RenderGraph has multiple resources bound to color attachment " + std::to_string(index) + " on target " + target.name);
                        desc.has_color[index] = true;
                        desc.colors[index] = state.desc;
                        break;
                    }
                    case ResourceAttachment::Depth:
                        if (desc.has_depth)
                            throw std::runtime_error("RenderGraph has multiple depth resources bound to target " + target.name);
                        desc.has_depth = true;
                        desc.depth = state.desc;
                        break;
                    case ResourceAttachment::DepthStencil:
                        if (desc.has_depth_stencil)
                            throw std::runtime_error("RenderGraph has multiple depth-stencil resources bound to target " + target.name);
                        desc.has_depth_stencil = true;
                        desc.depth_stencil = state.desc;
                        break;
                    default:
                        break;
                    }
                }

                if (isFrameBufferDescEmpty(desc))
                    continue;

                const bool recreate = !target_state.framebuffer ||
                                      !samePixelSize(target_state.size, m_frame_size) ||
                                      !sameFrameBufferDesc(target_state.desc, desc);
                if (recreate)
                {
                    if (target_state.framebuffer)
                        target_state.framebuffer->destroyGPU();
                    target_state.size = m_frame_size;
                    target_state.desc = desc;
                    target_state.framebuffer = createFrameBuffer(desc);
                }
                continue;
            }

            if (target.kind == RenderTargetKind::Backbuffer)
            {
                const bool recreate = !target_state.framebuffer ||
                                      !samePixelSize(target_state.size, m_frame_size);
                if (recreate)
                {
                    target_state.size = m_frame_size;
                    target_state.desc = {};
                    target_state.framebuffer = createBackbufferFrameBuffer();
                }
                continue;
            }

            if (target.kind == RenderTargetKind::CubeDepth)
            {
                target_state.size = m_frame_size;
                ensureCubeDepthTarget(target_state);
            }
        }
    }
}

void RenderGraph::destroyCubeDepthTarget(RenderTargetState& state)
{
    for (unsigned id : state.cube_maps)
    {
        if (id != 0)
            glDeleteTextures(1, &id);
    }
    state.cube_maps.clear();

    if (state.cube_framebuffer != 0)
    {
        glDeleteFramebuffers(1, &state.cube_framebuffer);
        state.cube_framebuffer = 0;
    }
    state.cube_edge = 0;
}

void RenderGraph::ensureCubeDepthTarget(RenderTargetState& state)
{
    const int cube_edge = std::clamp(std::min((int)m_frame_size.x, (int)m_frame_size.y), 256, 4096);

    if (state.cube_framebuffer == 0)
    {
        state.cube_framebuffer = m_rhi->newFramebufferHandle();
        m_rhi->bindFramebuffer(state.cube_framebuffer);
        m_rhi->setFramebufferDrawReadNone();
    }

    if (!state.cube_maps.empty() && state.cube_edge == cube_edge && state.cube_maps.size() >= (size_t)state.initial_cube_map_count)
        return;

    for (unsigned id : state.cube_maps)
    {
        if (id != 0)
            glDeleteTextures(1, &id);
    }

    state.cube_edge = cube_edge;
    const size_t count = std::max<size_t>(state.initial_cube_map_count, state.cube_maps.size());
    state.cube_maps.assign(count, 0);
    for (size_t i = 0; i < state.cube_maps.size(); ++i)
        state.cube_maps[i] = m_rhi->newDepthCubeMap(state.cube_edge);
}

void RenderGraph::ensureCubeDepthTargetCapacity(RenderPass::Type type, const std::string& target_name, size_t count)
{
    RenderTargetState* state = targetState(type, target_name);
    if (!state || state->kind != RenderTargetKind::CubeDepth)
        return;
    ensureCubeDepthTarget(*state);
    if (state->cube_maps.size() >= count)
        return;

    m_rhi->bindFramebuffer(state->cube_framebuffer);
    const size_t old_size = state->cube_maps.size();
    state->cube_maps.resize(count, 0);
    for (size_t i = old_size; i < state->cube_maps.size(); ++i)
        state->cube_maps[i] = m_rhi->newDepthCubeMap(state->cube_edge);
}

void RenderGraph::clearPassTargets(const PassNode& node)
{
    for (const PassNode::TargetDeclaration& target : node.m_targets)
    {
        TargetKey key{ node.m_type, target.name };
        RenderTargetState* state = targetState(key.pass, key.name);
        if (state)
            clearTarget(key, *state);
    }
}

void RenderGraph::clearTarget(const TargetKey&, RenderTargetState& state)
{
    if (state.kind == RenderTargetKind::Texture || state.kind == RenderTargetKind::Backbuffer)
    {
        if (state.framebuffer)
        {
            state.framebuffer->bind();
            state.framebuffer->clear();
        }
        return;
    }

    if (state.kind == RenderTargetKind::CubeDepth)
    {
        if (state.cube_framebuffer == 0)
            return;
        m_rhi->bindFramebuffer(state.cube_framebuffer);
        for (const unsigned cube_map : state.cube_maps)
        {
            for (int face = 0; face < 6; ++face)
            {
                m_rhi->attachDepthCubeFace(cube_map, face);
                m_rhi->clearColorDepthStencil(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
        m_rhi->bindDefaultFramebuffer();
    }
}

std::unique_ptr<RhiFrameBuffer> RenderGraph::createFrameBuffer(const FrameBufferDesc& desc) const
{
    if (desc.has_depth && desc.has_depth_stencil)
        throw std::runtime_error("RenderGraph framebuffer cannot have both depth and depth-stencil attachments");

    int sample_count = 0;
    auto updateSampleCount = [&sample_count](const ResourceDesc& resource_desc)
    {
        if (resource_desc.format == RhiTexture::Format::UnknownFormat)
            throw std::runtime_error("RenderGraph resource has unknown texture format");
        if (sample_count == 0)
        {
            sample_count = resource_desc.sample_count;
            return;
        }
        if (sample_count != resource_desc.sample_count)
            throw std::runtime_error("RenderGraph framebuffer attachments use mixed sample counts");
    };

    for (size_t i = 0; i < desc.has_color.size(); ++i)
    {
        if (desc.has_color[i])
            updateSampleCount(desc.colors[i]);
    }
    if (desc.has_depth)
        updateSampleCount(desc.depth);
    if (desc.has_depth_stencil)
        updateSampleCount(desc.depth_stencil);
    if (sample_count == 0)
        sample_count = 1;

    auto makeAttachment = [this](const ResourceDesc& resource_desc)
    {
        RhiTexture* texture = m_rhi->newTexture(resource_desc.format, m_frame_size, resource_desc.sample_count);
        texture->create();
        return RhiAttachment(texture);
    };

    std::array<RhiAttachment, 8> color_attachments;
    bool has_color = false;
    for (size_t i = 0; i < desc.has_color.size(); ++i)
    {
        if (!desc.has_color[i])
            continue;
        color_attachments[i] = makeAttachment(desc.colors[i]);
        has_color = true;
    }

    std::unique_ptr<RhiFrameBuffer> framebuffer(m_rhi->newFrameBuffer(RhiAttachment(), m_frame_size, sample_count));
    if (has_color)
        framebuffer->setColorAttachments(color_attachments);
    if (desc.has_depth)
        framebuffer->setDepthAttachment(makeAttachment(desc.depth));
    if (desc.has_depth_stencil)
        framebuffer->setDepthStencilAttachment(makeAttachment(desc.depth_stencil));
    framebuffer->create();
    return framebuffer;
}

std::unique_ptr<RhiFrameBuffer> RenderGraph::createBackbufferFrameBuffer() const
{
    return std::unique_ptr<RhiFrameBuffer>(m_rhi->newFrameBuffer(RhiAttachment(), m_frame_size));
}

RhiTexture* RenderGraph::texture(const std::string& resource_name) const
{
    auto it = m_resources.find(resource_name);
    if (it == m_resources.end())
        return nullptr;

    RhiFrameBuffer* framebuffer = frameBuffer(resource_name);
    if (!framebuffer)
        return nullptr;

    const RhiAttachment* attachment = nullptr;
    switch (it->second.binding.attachment)
    {
    case ResourceAttachment::Color:
        attachment = framebuffer->colorAttachmentAt(it->second.binding.color_attachment);
        break;
    case ResourceAttachment::Depth:
        attachment = framebuffer->depthAttachment();
        break;
    case ResourceAttachment::DepthStencil:
        attachment = framebuffer->depthStencilAttachment();
        break;
    default:
        break;
    }

    return attachment ? attachment->texture() : nullptr;
}

RhiFrameBuffer* RenderGraph::frameBuffer(const std::string& resource_name) const
{
    auto it = m_resources.find(resource_name);
    if (it == m_resources.end())
        return nullptr;
    return targetFrameBuffer(it->second.owner_pass, it->second.target);
}

bool RenderGraph::readPixelRGBA(const std::string& resource_name, int x, int y, unsigned char out_rgba[4]) const
{
    RhiFrameBuffer* framebuffer = frameBuffer(resource_name);
    if (!framebuffer)
        return false;

    m_rhi->readPixelRGBA(framebuffer->id(), x, y, out_rgba);
    return true;
}

RenderGraph::PassNode* RenderGraph::findNode(RenderPass::Type type)
{
    auto it = m_node_indices.find(type);
    return it == m_node_indices.end() ? nullptr : &m_nodes[it->second];
}

const RenderGraph::PassNode* RenderGraph::findNode(RenderPass::Type type) const
{
    auto it = m_node_indices.find(type);
    return it == m_node_indices.end() ? nullptr : &m_nodes[it->second];
}

void RenderGraph::resolveResourceDependencies()
{
    m_resources.clear();

    for (PassNode& node : m_nodes)
    {
        node.m_resolved_dependencies.clear();

        const bool clear_writes = !node.m_enabled && node.m_disabled_execution == DisabledExecution::Clear;
        if (node.m_enabled)
        {
            for (const std::string& resource_name : node.m_reads)
                resolveRead(node, resource_name);
            for (const std::string& resource_name : node.m_read_writes)
                resolveReadWrite(node, resource_name);
        }

        if (node.m_enabled || clear_writes)
        {
            for (const auto& write : node.m_writes)
                resolveWrite(node, write);
        }
    }
}

void RenderGraph::addResolvedDependency(PassNode& node, RenderPass::Type type)
{
    if (type == node.m_type)
        return;
    if (std::find(node.m_resolved_dependencies.begin(), node.m_resolved_dependencies.end(), type) == node.m_resolved_dependencies.end())
        node.m_resolved_dependencies.push_back(type);
}

void RenderGraph::resolveRead(PassNode& node, const std::string& resource_name)
{
    auto it = m_resources.find(resource_name);
    if (it == m_resources.end())
        throw std::runtime_error("RenderGraph resource is read before it is written: " + resource_name);

    addResolvedDependency(node, it->second.last_modifier);
}

void RenderGraph::resolveReadWrite(PassNode& node, const std::string& resource_name)
{
    auto it = m_resources.find(resource_name);
    if (it == m_resources.end())
        throw std::runtime_error("RenderGraph resource is read/written before it is written: " + resource_name);

    addResolvedDependency(node, it->second.last_modifier);
    it->second.last_modifier = node.m_type;
}

void RenderGraph::resolveWrite(PassNode& node, const PassNode::ResourceWrite& write)
{
    m_resources[write.name] = ResourceState{ node.m_type, node.m_type, write.target, write.binding, write.desc };
}

void RenderGraph::visit(RenderPass::Type type, std::unordered_set<RenderPass::Type>& visiting, std::unordered_set<RenderPass::Type>& visited)
{
    if (visited.find(type) != visited.end())
        return;
    if (visiting.find(type) != visiting.end())
        throw std::runtime_error("RenderGraph contains a cycle");

    PassNode* node = findNode(type);
    if (!node)
        return;

    visiting.insert(type);
    if (node->m_enabled)
    {
        for (RenderPass::Type dependency : node->m_resolved_dependencies)
            visit(dependency, visiting, visited);
    }
    visiting.erase(type);

    visited.insert(type);
    m_compiled_order.push_back(type);
}
