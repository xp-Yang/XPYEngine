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
    writeToScene(context);
}

void BloomPass::extractBright(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.targetFrameBuffer(RGTarget::Main);
    if (!framebuffer)
        return;
    framebuffer->bind();
    framebuffer->clear();

    RhiTexture* lighted_texture = context.texture(RGResource::SceneColor);
    auto lighted_map = lighted_texture ? lighted_texture->id() : 0;
    static RenderShaderObject* extract_bright_shader = RenderShaderObject::getShaderObject(ShaderType::ExtractBrightShader);
    extract_bright_shader->start_using();
    extract_bright_shader->setTexture("Texture", 0, lighted_map);
    m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());
    extract_bright_shader->stop_using();
}

void BloomPass::blur(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.targetFrameBuffer(RGTarget::Main);
    RhiFrameBuffer* pingpong1_framebuffer = context.targetFrameBuffer(RGTarget::BloomPingPong1);
    RhiFrameBuffer* pingpong2_framebuffer = context.targetFrameBuffer(RGTarget::BloomPingPong2);
    if (!framebuffer || !pingpong1_framebuffer || !pingpong2_framebuffer)
        return;

    pingpong1_framebuffer->bind();
    pingpong1_framebuffer->clear();
    pingpong2_framebuffer->bind();
    pingpong2_framebuffer->clear();

    auto bright_map = framebuffer->colorAttachmentAt(0)->texture()->id();

    static RenderShaderObject* blur_shader = RenderShaderObject::getShaderObject(ShaderType::GaussianBlur);
    bool horizontal = true;
    unsigned int amount = 16;
    blur_shader->start_using();
    for (unsigned int i = 0; i < amount; i++)
    {
        if (horizontal)
            pingpong1_framebuffer->bind();
        else
            pingpong2_framebuffer->bind();
        unsigned int map = horizontal ?
            pingpong2_framebuffer->colorAttachmentAt(0)->texture()->id() :
            pingpong1_framebuffer->colorAttachmentAt(0)->texture()->id();
        blur_shader->setTexture("image", 0, (i == 0) ? bright_map : map);
        blur_shader->setInt("horizontal", horizontal);
        m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());
        horizontal = !horizontal;
    }
    blur_shader->stop_using();
}

void BloomPass::writeToScene(RenderPassContext& context)
{
    RhiFrameBuffer* scene_framebuffer = context.frameBuffer(RGResource::SceneColor);
    scene_framebuffer->bind();

    static RenderShaderObject* bloom_shader = RenderShaderObject::getShaderObject(ShaderType::BloomShader);
    bloom_shader->start_using();
    RhiTexture* lighted_texture = context.texture(RGResource::SceneColor);
    auto lighted_map = lighted_texture ? lighted_texture->id() : 0;
    bloom_shader->setTexture("Texture", 0, lighted_map);
    RhiTexture* blurred_bright_texture = context.texture(RGResource::BloomPingPong1Color);
    bloom_shader->setTexture("bloomMap", 1, blurred_bright_texture->id());
    m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());
}
