#include "BloomPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

BloomPass::BloomPass()
{
    m_type = RenderPass::Type::Bloom;
}

void BloomPass::draw(RenderPassContext& context)
{
    extractBright(context);
    blur(context);
}

void BloomPass::extractBright(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.targetFrameBuffer();
    if (!framebuffer)
        return;
    framebuffer->bind();
    framebuffer->clear();

    RhiTexture* lighted_texture = context.readTexture(RGSlot::Source);
    auto lighted_map = lighted_texture ? lighted_texture->id() : 0;
    static RenderShaderObject* extract_bright_shader = RenderShaderObject::getShaderObject(ShaderType::ExtractBrightShader);
    extract_bright_shader->start_using();
    extract_bright_shader->setTexture("Texture", 0, lighted_map);
    m_rhi->drawIndexed(m_render_source_data->screen_quad->getVAO(), m_render_source_data->screen_quad->indicesCount());
    extract_bright_shader->stop_using();
}

void BloomPass::blur(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.targetFrameBuffer();
    RhiFrameBuffer* pingpong_framebuffer = context.targetFrameBuffer(RGTarget::BloomPingPong);
    if (!framebuffer || !pingpong_framebuffer)
        return;

    pingpong_framebuffer->bind();
    pingpong_framebuffer->clear();

    auto bright_map = framebuffer->colorAttachmentAt(0)->texture()->id();
    unsigned int map = pingpong_framebuffer->colorAttachmentAt(0)->texture()->id();

    static RenderShaderObject* blur_shader = RenderShaderObject::getShaderObject(ShaderType::GaussianBlur);
    bool horizontal = true;
    unsigned int amount = 8;
    blur_shader->start_using();
    for (unsigned int i = 0; i < amount; i++)
    {
        blur_shader->setTexture("image", 0, (i == 0) ? bright_map : map);
        blur_shader->setInt("horizontal", horizontal);
        m_rhi->drawIndexed(m_render_source_data->screen_quad->getVAO(), m_render_source_data->screen_quad->indicesCount());
        horizontal = !horizontal;
    }
    blur_shader->stop_using();

    pingpong_framebuffer->blitTo(framebuffer, RhiTexture::Format::RGB16F);
}
