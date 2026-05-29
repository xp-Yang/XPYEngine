#include "ToneMappingPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

ToneMappingPass::ToneMappingPass()
{
    m_type = RenderPass::Type::ToneMapping;
}

ToneMappingPass::~ToneMappingPass()
{
    destroyTempBuffer();
}

void ToneMappingPass::ensureTempBuffer(const Vec2& scene_size)
{
    if (m_current_scene_size == scene_size && m_temp_framebuffer)
        return;

    destroyTempBuffer();
    m_current_scene_size = scene_size;

    m_temp_texture = m_rhi->newTexture(RhiTexture::Format::RGBA16F, scene_size, 1);
    m_temp_texture->create();

    RhiFrameBuffer* fb = m_rhi->newFrameBuffer(RhiAttachment(), scene_size, 1);
    std::array<RhiAttachment, 8> attachments{};
    attachments[0] = RhiAttachment(m_temp_texture, 0, 0, false);
    fb->setColorAttachments(attachments);
    fb->create();
    m_temp_framebuffer.reset(fb);
}

void ToneMappingPass::destroyTempBuffer()
{
    if (m_temp_framebuffer)
        m_temp_framebuffer->destroyGPU();
    m_temp_framebuffer.reset();

    if (m_temp_texture)
    {
        m_temp_texture->destroy();
        delete m_temp_texture;
        m_temp_texture = nullptr;
    }

    m_current_scene_size = Vec2(0.f, 0.f);
}

void ToneMappingPass::draw(RenderPassContext& context)
{
    RhiTexture* scene_color = context.texture(RGResource::SceneColor);
    RhiFrameBuffer* scene_fbo = context.frameBuffer(RGResource::SceneColor);
    if (!scene_color || !scene_fbo)
        return;

    ensureTempBuffer(scene_color->pixelSize());
    if (!m_temp_framebuffer)
        return;

    auto* quad_layout = context.builtinResources().screen_quad->vertexLayout();
    const int quad_indices = static_cast<int>(context.builtinResources().screen_quad->indicesCount());

    m_command_buffer->beginPass(m_temp_framebuffer.get(), Color4(0.f, 0.f, 0.f, 1.f));

    RenderPipelineState state;
    state.depthTest = false;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(
        RenderPipelineLibrary::graphicsPipeline(ShaderType::ToneMappingShader, state));

    ShaderResourceBindings bindings;
    bindings.setTexture("sceneColor", 0, scene_color);
    bindings.setFloat("exposure", m_exposure);
    m_command_buffer->setShaderResources(&bindings);

    m_command_buffer->setVertexInput(quad_layout);
    m_command_buffer->drawIndexed(quad_indices);
    m_command_buffer->endPass();

    m_command_buffer->blit(m_temp_framebuffer.get(), scene_fbo, RhiTexture::Format::RGBA16F);
}
