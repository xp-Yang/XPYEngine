#include "BloomPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

BloomPass::BloomPass()
{
    m_type = RenderPass::Type::Bloom;
}

BloomPass::~BloomPass()
{
    destroyMipChain();
}

void BloomPass::draw(RenderPassContext& context)
{
    RhiTexture* scene_color = context.texture(RGResource::SceneColor);
    if (!scene_color)
        return;

    ensureMipChain(scene_color->pixelSize());
    if (m_mip_chain.empty())
        return;

    downsample(context);
    upsample(context);
    composite(context);
}

void BloomPass::ensureMipChain(const Vec2& scene_size)
{
    const int mip_levels = std::clamp(m_params.mipLevels, MIN_MIP_LEVELS, MAX_MIP_LEVELS);

    if (m_current_scene_size == scene_size && m_current_mip_count == mip_levels)
        return;

    destroyMipChain();

    m_current_scene_size = scene_size;
    m_current_mip_count = mip_levels;

    Vec2 mip_size = Vec2(
        std::floor(scene_size.x / 2.0f),
        std::floor(scene_size.y / 2.0f)
    );

    for (int i = 0; i < mip_levels; ++i)
    {
        if (mip_size.x < 1.0f || mip_size.y < 1.0f)
            break;

        MipLevel level;
        level.size = mip_size;
        level.texture = m_rhi->newTexture(RhiTexture::Format::RGB16F, mip_size, 1);
        level.texture->create();

        RhiFrameBuffer* fb = m_rhi->newFrameBuffer(RhiAttachment(), mip_size, 1);
        std::array<RhiAttachment, 8> color_attachments{};
        color_attachments[0] = RhiAttachment(level.texture, 0, 0, false);
        fb->setColorAttachments(color_attachments);
        fb->create();
        level.framebuffer.reset(fb);

        m_mip_chain.push_back(std::move(level));

        mip_size = Vec2(
            std::floor(mip_size.x / 2.0f),
            std::floor(mip_size.y / 2.0f)
        );
    }

    // Composite temp buffer at full scene resolution
    m_composite_texture = m_rhi->newTexture(RhiTexture::Format::RGBA16F, scene_size, 1);
    m_composite_texture->create();

    RhiFrameBuffer* cfb = m_rhi->newFrameBuffer(RhiAttachment(), scene_size, 1);
    std::array<RhiAttachment, 8> comp_attachments{};
    comp_attachments[0] = RhiAttachment(m_composite_texture, 0, 0, false);
    cfb->setColorAttachments(comp_attachments);
    cfb->create();
    m_composite_framebuffer.reset(cfb);
}

void BloomPass::destroyMipChain()
{
    for (auto& level : m_mip_chain)
    {
        if (level.framebuffer)
            level.framebuffer->destroyGPU();
        if (level.texture)
        {
            level.texture->destroy();
            delete level.texture;
        }
    }
    m_mip_chain.clear();

    if (m_composite_framebuffer)
        m_composite_framebuffer->destroyGPU();
    m_composite_framebuffer.reset();

    if (m_composite_texture)
    {
        m_composite_texture->destroy();
        delete m_composite_texture;
        m_composite_texture = nullptr;
    }

    m_current_scene_size = Vec2(0.f, 0.f);
    m_current_mip_count = 0;
}

void BloomPass::downsample(RenderPassContext& context)
{
    RhiTexture* scene_color = context.texture(RGResource::SceneColor);
    auto* quad_layout = context.builtinResources().screen_quad->vertexLayout();
    const int quad_indices = static_cast<int>(context.builtinResources().screen_quad->indicesCount());

    RhiTexture* src_texture = scene_color;

    for (size_t i = 0; i < m_mip_chain.size(); ++i)
    {
        MipLevel& dst = m_mip_chain[i];
        Vec2 src_size = (i == 0) ? m_current_scene_size : m_mip_chain[i - 1].size;

        m_command_buffer->beginPass(dst.framebuffer.get(), Color4(0.f, 0.f, 0.f, 1.f));

        m_command_buffer->setGraphicsPipeline(
            RenderPipelineLibrary::graphicsPipeline(ShaderType::BloomDownsampleShader));

        ShaderResourceBindings bindings;
        bindings.setTexture("source", 0, src_texture);
        bindings.setFloat3("texelSize", Vec3(1.0f / src_size.x, 1.0f / src_size.y, 0.0f));
        bindings.setBool("applyThreshold", i == 0);
        bindings.setFloat("threshold", m_params.threshold);
        bindings.setFloat("softKnee", m_params.softKnee);
        m_command_buffer->setShaderResources(&bindings);

        m_command_buffer->setVertexInput(quad_layout);
        m_command_buffer->drawIndexed(quad_indices);
        m_command_buffer->endPass();

        src_texture = dst.texture;
    }
}

void BloomPass::upsample(RenderPassContext& context)
{
    if (m_mip_chain.size() < 2)
        return;

    auto* quad_layout = context.builtinResources().screen_quad->vertexLayout();
    const int quad_indices = static_cast<int>(context.builtinResources().screen_quad->indicesCount());

    RenderPipelineState state;
    state.depthTest = false;
    state.depthWrite = false;

    for (int i = static_cast<int>(m_mip_chain.size()) - 2; i >= 0; --i)
    {
        MipLevel& src = m_mip_chain[i + 1];
        MipLevel& dst = m_mip_chain[i];

        m_command_buffer->beginPass(dst.framebuffer.get(), Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

        m_command_buffer->setGraphicsPipeline(
            RenderPipelineLibrary::graphicsPipeline(ShaderType::BloomUpsampleShader, state));

        ShaderResourceBindings bindings;
        bindings.setTexture("source", 0, src.texture);
        bindings.setTexture("destination", 1, dst.texture);
        bindings.setFloat3("texelSize", Vec3(1.0f / src.size.x, 1.0f / src.size.y, 0.0f));
        bindings.setFloat("blendFactor", 0.7f);
        m_command_buffer->setShaderResources(&bindings);

        m_command_buffer->setVertexInput(quad_layout);
        m_command_buffer->drawIndexed(quad_indices);
        m_command_buffer->endPass();
    }
}

void BloomPass::composite(RenderPassContext& context)
{
    RhiTexture* scene_color = context.texture(RGResource::SceneColor);
    if (!scene_color || !m_composite_framebuffer || m_mip_chain.empty())
        return;

    auto* quad_layout = context.builtinResources().screen_quad->vertexLayout();
    const int quad_indices = static_cast<int>(context.builtinResources().screen_quad->indicesCount());

    // Render composite to temp buffer to avoid SceneColor read-write feedback loop
    m_command_buffer->beginPass(m_composite_framebuffer.get(), Color4(0.f, 0.f, 0.f, 1.f));

    RenderPipelineState state;
    state.depthTest = false;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(
        RenderPipelineLibrary::graphicsPipeline(ShaderType::BloomCompositeShader, state));

    ShaderResourceBindings bindings;
    bindings.setTexture("sceneColor", 0, scene_color);
    bindings.setTexture("bloomTexture", 1, m_mip_chain[0].texture);
    bindings.setFloat("bloomIntensity", m_params.intensity);
    m_command_buffer->setShaderResources(&bindings);

    m_command_buffer->setVertexInput(quad_layout);
    m_command_buffer->drawIndexed(quad_indices);
    m_command_buffer->endPass();

    // Blit result back to SceneColor FBO
    RhiFrameBuffer* scene_fbo = context.frameBuffer(RGResource::SceneColor);
    if (scene_fbo)
        m_command_buffer->blit(m_composite_framebuffer.get(), scene_fbo, RhiTexture::Format::RGB16F);
}
