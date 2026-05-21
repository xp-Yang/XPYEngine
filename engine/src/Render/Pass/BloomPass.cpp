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
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);
    if (!framebuffer)
        return;
    m_command_buffer->beginPass(framebuffer);

    RhiTexture* lighted_texture = context.texture(RGResource::SceneColor);
    GL_HANDLE lighted_map = lighted_texture ? lighted_texture->id() : 0;
    static RenderShaderObject* extract_bright_shader = RenderShaderObject::getShaderObject(ShaderType::ExtractBrightShader);
    m_command_buffer->setGraphicsPipeline(extract_bright_shader->graphicsPipeline());
    extract_bright_shader->setTexture("Texture", 0, lighted_map);
    m_command_buffer->setVertexInput(context.renderSourceData().screen_quad->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(context.renderSourceData().screen_quad->indicesCount()));
    m_command_buffer->endPass();
}

void BloomPass::blur(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);
    RhiFrameBuffer* pingpong1_framebuffer = context.frameBufferOfTarget(RGTarget::BloomPingPong1);
    RhiFrameBuffer* pingpong2_framebuffer = context.frameBufferOfTarget(RGTarget::BloomPingPong2);
    if (!framebuffer || !pingpong1_framebuffer || !pingpong2_framebuffer)
        return;

    m_command_buffer->beginPass(pingpong1_framebuffer);
    m_command_buffer->endPass();
    m_command_buffer->beginPass(pingpong2_framebuffer);
    m_command_buffer->endPass();

    GL_HANDLE bright_map = framebuffer->colorAttachmentAt(0)->texture()->id();

    static RenderShaderObject* blur_shader = RenderShaderObject::getShaderObject(ShaderType::GaussianBlur);
    bool horizontal = true;
    unsigned int amount = 16;
    for (unsigned int i = 0; i < amount; i++)
    {
        RhiFrameBuffer* target = horizontal ? pingpong1_framebuffer : pingpong2_framebuffer;
        m_command_buffer->beginPass(target, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);
        m_command_buffer->setGraphicsPipeline(blur_shader->graphicsPipeline());
        GL_HANDLE map = horizontal ?
            pingpong2_framebuffer->colorAttachmentAt(0)->texture()->id() :
            pingpong1_framebuffer->colorAttachmentAt(0)->texture()->id();
        blur_shader->setTexture("image", 0, (i == 0) ? bright_map : map);
        blur_shader->setInt("horizontal", horizontal);
        m_command_buffer->setVertexInput(context.renderSourceData().screen_quad->vertexLayout());
        m_command_buffer->drawIndexed(static_cast<int>(context.renderSourceData().screen_quad->indicesCount()));
        m_command_buffer->endPass();
        horizontal = !horizontal;
    }
}

void BloomPass::writeToScene(RenderPassContext& context)
{
    RhiFrameBuffer* scene_framebuffer = context.frameBuffer(RGResource::SceneColor);
    m_command_buffer->beginPass(scene_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    static RenderShaderObject* bloom_shader = RenderShaderObject::getShaderObject(ShaderType::BloomShader);
    RenderPipelineState state;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(bloom_shader->graphicsPipeline(state));
    RhiTexture* lighted_texture = context.texture(RGResource::SceneColor);
    GL_HANDLE lighted_map = lighted_texture ? lighted_texture->id() : 0;
    bloom_shader->setTexture("Texture", 0, lighted_map);
    RhiTexture* blurred_bright_texture = context.texture(RGResource::BloomPingPong1Color);
    bloom_shader->setTexture("bloomMap", 1, blurred_bright_texture->id());
    m_command_buffer->setVertexInput(context.renderSourceData().screen_quad->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(context.renderSourceData().screen_quad->indicesCount()));
    m_command_buffer->endPass();
}
