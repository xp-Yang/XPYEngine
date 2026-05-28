#include "RenderGraph.hpp"
#include "RenderPassContext.hpp"

static bool samePixelSize(const Vec2& lhs, const Vec2& rhs)
{
    return (int)lhs.x == (int)rhs.x && (int)lhs.y == (int)rhs.y;
}

RenderGraph::~RenderGraph()
{
    // TODO 移除gl依赖
    for (auto& pair : m_render_targets)
        destroyCubeShadowFrameBuffer(pair.second);
}

void RenderGraph::reset()
{
    m_nodes.clear();
    m_ordered_nodes.clear();
    m_outputs.clear();
    m_resources.clear();
    // TODO m_targets释放
}

void RenderGraph::setFrameSize(const Vec2& pixel_size)
{
    m_frame_size = pixel_size;
}

RenderGraphPassNode& RenderGraph::addPass(RenderPass::Type type, RenderPass* pass)
{
    assert(pass);

    const size_t index = m_nodes.size();
    m_nodes.emplace_back(type, pass);
    return m_nodes.back();
}

void RenderGraph::markOutput(const RGResourceName& resource_name)
{
    if (std::find(m_outputs.begin(), m_outputs.end(), resource_name) == m_outputs.end())
        m_outputs.push_back(resource_name);
}

void RenderGraph::visit(RenderPass::Type type, std::unordered_set<RenderPass::Type>& visiting, std::unordered_set<RenderPass::Type>& visited)
{
    if (visited.find(type) != visited.end())
        return;
    if (visiting.find(type) != visiting.end())
        throw std::runtime_error("RenderGraph contains a cycle");

    auto it = std::find_if(m_nodes.begin(), m_nodes.end(), [type](const auto& node) { return node.m_type == type; });
    if (it == m_nodes.end())
        return;

    RenderGraphPassNode* node = &(*it);

    visiting.insert(type);
    if (node->m_enabled)
    {
        for (RenderPass::Type dependency : node->m_resolved_dependencies)
            visit(dependency, visiting, visited);
    }
    visiting.erase(type);

    visited.insert(type);
    m_ordered_nodes.push_back(node);
}

void RenderGraph::compile()
{
    m_ordered_nodes.clear();
    resolveResourceDependencies();

    std::unordered_set<RenderPass::Type> visiting;
    std::unordered_set<RenderPass::Type> visited;

    // 如果没有标记 output，它会尝试编译所有 pass。
    // 如果标记了 output，它只从 output 反向找依赖，所以没被最终输出用到的 pass 会被裁剪掉。
    if (!m_outputs.empty())
    {
        for (const RGResourceName& resource_name : m_outputs)
        {
            auto it = m_resources.find(resource_name);
            if (it == m_resources.end())
                throw std::runtime_error("RenderGraph output resource is not written: " + resource_name);
            visit(it->second.last_modifier_pass, visiting, visited);
        }
    }
    else
    {
        for (const auto& node : m_nodes)
            visit(node.m_type, visiting, visited);
    }

    ensureRenderTargets();
}

void RenderGraph::execute(RenderScene& render_scene, RenderFrameData& frame_data, RenderBuiltinResources& builtin_resources)
{
    for (RenderGraphPassNode* node : m_ordered_nodes)
    {
        if (!node || !node->m_pass)
            continue;

        if (!node->m_enabled)
        {
            if (node->m_disabled_execution == RGDisabledExecution::Clear)
                clearPassTargets(*node);
            continue;
        }

        if (node->m_setup)
            node->m_setup(*node->m_pass);

        RenderPassContext context(*this, *node, render_scene, frame_data, builtin_resources);
        node->m_pass->draw(context);
    }
}

const RenderGraphRenderTarget* RenderGraph::findRenderTarget(RenderPass::Type type, const RGTargetName& target_name) const
{
    auto it = m_render_targets.find(TargetKey{ type, target_name });
    return it == m_render_targets.end() ? nullptr : &it->second;
}

RenderGraphRenderTarget* RenderGraph::findRenderTarget(RenderPass::Type type, const RGTargetName& target_name)
{
    auto it = m_render_targets.find(TargetKey{ type, target_name });
    return it == m_render_targets.end() ? nullptr : &it->second;
}

std::vector<RhiTexture*> RenderGraph::cubeShadowMapsOf(RenderPass::Type type) const
{
    std::vector<RhiTexture*> res;
    const RenderGraphRenderTarget* target = findRenderTarget(type, RGTarget::ShadowPointDepth);
    if (!target)
        return res;

    res.resize(target->cube_shadow_framebuffers.size());
    for (size_t i = 0; i < target->cube_shadow_framebuffers.size(); i++) {
        res[i] = target->cube_shadow_framebuffers[i][0]->depthAttachment()->texture();
    }
    return res;
}

void RenderGraph::ensureRenderTargets()
{
    for (RenderGraphPassNode* node_ : m_ordered_nodes)
    {
        auto& node = *node_;
        for (const RenderTargetDeclaration& target_decl : node.m_targets)
        {
            RenderGraphRenderTarget& target = m_render_targets[TargetKey{ node.m_type, target_decl.name }];

            if (target.target_decl.render_target_type != target_decl.render_target_type) {
                if (target.framebuffer)
                    target.framebuffer->destroyGPU();
                target.framebuffer.reset();
                destroyCubeShadowFrameBuffer(target);
            }
            target.target_decl = target_decl;
        }
    }

    for (RenderGraphPassNode* node_ : m_ordered_nodes)
    {
        auto& node = *node_;
        for (const RenderTargetDeclaration& target_decl : node.m_targets)
        {
            RenderGraphRenderTarget& target = m_render_targets[TargetKey{ node.m_type, target_decl.name }];

            RhiFrameBufferDesc framebuffer_desc;
            framebuffer_desc.size = m_frame_size;

            if (target_decl.render_target_type == RenderTargetType::FrameBuffer)
            {
                for (const auto& resource_decl : node.m_resources) {
                    if (resource_decl.owner_target_name != target_decl.name)
                        continue;
                    else {
                        switch (resource_decl.attachment_desc.attachment_type)
                        {
                        case RhiAttachment::Type::Color:
                        {
                            const int index = resource_decl.attachment_desc.color_attachment_index;
                            if (index < 0 || index >= (int)framebuffer_desc.has_color.size())
                                throw std::runtime_error("RenderGraph color attachment index is out of range: " + target_decl.name);
                            if (framebuffer_desc.has_color[index])
                                throw std::runtime_error("RenderGraph has multiple resources bound to color attachment " + std::to_string(index) + " on target " + target_decl.name);
                            framebuffer_desc.has_color[index] = true;
                            framebuffer_desc.colors[index] = resource_decl.attachment_desc;
                            break;
                        }
                        case RhiAttachment::Type::Depth:
                            if (framebuffer_desc.has_depth)
                                throw std::runtime_error("RenderGraph has multiple depth resources bound to target " + target_decl.name);
                            framebuffer_desc.has_depth = true;
                            framebuffer_desc.depth = resource_decl.attachment_desc;
                            break;
                        case RhiAttachment::Type::DepthStencil:
                            if (framebuffer_desc.has_depth_stencil)
                                throw std::runtime_error("RenderGraph has multiple depth-stencil resources bound to target " + target_decl.name);
                            framebuffer_desc.has_depth_stencil = true;
                            framebuffer_desc.depth_stencil = resource_decl.attachment_desc;
                            break;
                        default:
                            break;
                        }
                    }
                }

                if (framebuffer_desc.isEmpty())
                    continue;

                const bool recreate = !target.framebuffer ||
                                      !framebuffer_desc.isSameWith(target.framebuffer_desc);
                if (recreate)
                {
                    if (target.framebuffer)
                        target.framebuffer->destroyGPU();
                    target.framebuffer_desc = framebuffer_desc;
                    target.framebuffer = createFrameBuffer(framebuffer_desc);
                }
                continue;
            }
            else if (target_decl.render_target_type == RenderTargetType::ScreenFrameBuffer)
            {
                const bool recreate = !target.framebuffer ||
                                      !samePixelSize(target.framebuffer_desc.size, m_frame_size);
                if (recreate)
                {
                    target.framebuffer_desc = {};
                    target.framebuffer_desc.size = m_frame_size;
                    target.framebuffer = createDefaultFrameBuffer();
                }
                continue;
            }
            else if (target_decl.render_target_type == RenderTargetType::CubeDepth)
            {
                target.framebuffer_desc.size = m_frame_size;
            }
        }
    }
}

void RenderGraph::clearPassTargets(const RenderGraphPassNode& node)
{
    for (const RenderTargetDeclaration& decalred_target : node.m_targets)
    {
        TargetKey key{ node.m_type, decalred_target.name };
        RenderGraphRenderTarget* target = findRenderTarget(key.pass, key.name);
        if (target) {
            if (decalred_target.render_target_type == RenderTargetType::FrameBuffer || decalred_target.render_target_type == RenderTargetType::ScreenFrameBuffer)
            {
                if (target->framebuffer)
                {
                    target->framebuffer->bind();
                    target->framebuffer->clear();
                }
            }

            else if (decalred_target.render_target_type == RenderTargetType::CubeDepth)
            {
                for (auto& face_framebuffers : target->cube_shadow_framebuffers)
                {
                    for (auto& framebuffer : face_framebuffers)
                    {
                        if (framebuffer)
                            framebuffer->clear(Color4(1.0f, 1.0f, 1.0f, 1.0f));
                    }
                }
            }
        }
    }
}

std::unique_ptr<RhiFrameBuffer> RenderGraph::createFrameBuffer(const RhiFrameBufferDesc& desc) const
{
    if (desc.has_depth && desc.has_depth_stencil)
        throw std::runtime_error("RenderGraph framebuffer cannot have both depth and depth-stencil attachments");

    int sample_count = 0;
    auto updateSampleCount = [&sample_count](const RhiAttachmentDesc& resource_desc)
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

    auto makeAttachment = [this](const RhiAttachmentDesc& resource_desc)
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

std::unique_ptr<RhiFrameBuffer> RenderGraph::createDefaultFrameBuffer() const
{
    return std::unique_ptr<RhiFrameBuffer>(m_rhi->newFrameBuffer(RhiAttachment(), m_frame_size));
}

void RenderGraph::destroyCubeShadowFrameBuffer(RenderGraphRenderTarget& target)
{
    for (auto& face_framebuffers : target.cube_shadow_framebuffers)
    {
        for (auto& framebuffer : face_framebuffers)
        {
            if (framebuffer)
                framebuffer->destroyGPU();
            framebuffer.reset();
        }
    }
    target.cube_shadow_framebuffers.clear();

    // TODO 释放cube_shadow_framebuffers的texture资源，因为cube_shadow_framebuffers.depthAttachment不是这些texture的owner，不会释放
    //for (RhiTexture* tex : cube_textures)
    //{
    //    if (!tex)
    //        continue;
    //    tex->destroy();
    //    delete tex;
    //}
}

void RenderGraph::ensureCubeShadowMapsCount(RenderPass::Type type, size_t count)
{
    RenderGraphRenderTarget* target = findRenderTarget(type, RGTarget::ShadowPointDepth);
    if (!target || target->cube_shadow_framebuffers.size() >= count)
        return;

    while (target->cube_shadow_framebuffers.size() < count)
        appendCubeShadowMap(*target);
}

void RenderGraph::appendCubeShadowMap(RenderGraphRenderTarget& target)
{
    const int cube_edge = std::clamp(std::min((int)m_frame_size.x, (int)m_frame_size.y), 256, 4096);
    RhiTexture* cube_texture = m_rhi->newTexture(
        RhiTexture::Format::DEPTH,
        Vec2(cube_edge),
        1,
        static_cast<RhiTexture::Flag>(RhiTexture::RenderTarget | RhiTexture::CubeMap));
    cube_texture->create();

    std::array<std::unique_ptr<RhiFrameBuffer>, 6> face_framebuffers;
    for (int face = 0; face < 6; ++face)
    {
        std::unique_ptr<RhiFrameBuffer> framebuffer(m_rhi->newFrameBuffer(RhiAttachment(), Vec2(cube_edge), 1));
        framebuffer->setDepthAttachment(RhiAttachment(cube_texture, face, 0, false));
        framebuffer->create();
        face_framebuffers[face] = std::move(framebuffer);
    }

    target.cube_shadow_framebuffers.push_back(std::move(face_framebuffers));
}

RhiFrameBuffer* RenderGraph::cubeShadowFaceFrameBufferOf(RenderPass::Type type, size_t cube_index, int face) const
{
    if (face < 0 || face >= 6)
        return nullptr;

    const RenderGraphRenderTarget* target = findRenderTarget(type, RGTarget::ShadowPointDepth);
    if (!target || cube_index >= target->cube_shadow_framebuffers.size())
        return nullptr;

    return target->cube_shadow_framebuffers[cube_index][face].get();
}

RhiTexture* RenderGraph::textureOf(const RGResourceName& resource_name) const
{
    auto it = m_resources.find(resource_name);
    if (it == m_resources.end())
        return nullptr;

    RhiFrameBuffer* framebuffer = frameBufferOf(resource_name);
    if (!framebuffer)
        return nullptr;

    const RhiAttachment* attachment = nullptr;
    auto& resource = it->second;
    switch (resource.attachment_desc.attachment_type)
    {
    case RhiAttachment::Type::Color:
        attachment = framebuffer->colorAttachmentAt(resource.attachment_desc.color_attachment_index);
        break;
    case RhiAttachment::Type::Depth:
        attachment = framebuffer->depthAttachment();
        break;
    case RhiAttachment::Type::DepthStencil:
        attachment = framebuffer->depthStencilAttachment();
        break;
    default:
        break;
    }

    return attachment ? attachment->texture() : nullptr;
}

RhiFrameBuffer* RenderGraph::frameBufferOf(const RGResourceName& resource_name) const
{
    auto it = m_resources.find(resource_name);
    if (it == m_resources.end())
        return nullptr;
    auto& resource = it->second;
    const RenderGraphRenderTarget* target = findRenderTarget(resource.owner_pass, resource.owner_target_name);
    return target ? target->framebuffer.get() : nullptr;
}

bool RenderGraph::readPixelRGBAOf(const RGResourceName& resource_name, int x, int y, unsigned char out_rgba[4]) const
{
    RhiFrameBuffer* framebuffer = frameBufferOf(resource_name);
    if (!framebuffer)
        return false;

    m_rhi->readPixelRGBA(framebuffer->id(), x, y, out_rgba);
    return true;
}

void RenderGraph::resolveReads(RenderGraphPassNode& node)
{
    for (const RGResourceName& read : node.m_reads)
    {
        auto it = m_resources.find(read);
        if (it == m_resources.end())
            throw std::runtime_error("RenderGraph resource is read before it is produced: " + read);

        node.addResolvedDependency(it->second.last_modifier_pass);
    }
}

void RenderGraph::resolveModifies(RenderGraphPassNode& node)
{
    for (const RGResourceName& modify : node.m_modifies)
    { 
        auto it = m_resources.find(modify);
        if (it == m_resources.end())
            throw std::runtime_error("RenderGraph resource is modified before it is produced: " + modify);

        node.addResolvedDependency(it->second.last_modifier_pass);
        it->second.last_modifier_pass = node.m_type;
    }
}

void RenderGraph::resolveResources(RenderGraphPassNode& node)
{
    for (const auto& resource_decl : node.m_resources)
    {
        if (m_resources.find(resource_decl.name) != m_resources.end())
            throw std::runtime_error("RenderGraph resource is produced by multiple passes: " + resource_decl.name);
        m_resources[resource_decl.name] = resource_decl;
    }
}

void RenderGraph::resolveResourceDependencies()
{
    m_resources.clear();

    for (RenderGraphPassNode& node : m_nodes)
    {
        node.m_resolved_dependencies.clear();

        bool clear_writes = !node.m_enabled && node.m_disabled_execution == RGDisabledExecution::Clear;
        if (node.m_enabled || clear_writes)
            resolveResources(node);
    }

    for (RenderGraphPassNode& node : m_nodes) {
        if (node.m_enabled) {
            resolveReads(node);
            resolveModifies(node);
        }
    }
}
