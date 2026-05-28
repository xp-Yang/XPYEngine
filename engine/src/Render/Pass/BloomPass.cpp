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

    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::ExtractBrightShader));
    ShaderResourceBindings bindings;
    bindings.setTexture("Texture", 0, context.texture(RGResource::SceneColor));
    m_command_buffer->setShaderResources(&bindings);
    m_command_buffer->setVertexInput(context.builtinResources().screen_quad->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(context.builtinResources().screen_quad->indicesCount()));
    m_command_buffer->endPass();
}

void BloomPass::blur(RenderPassContext& context)
{
    RhiFrameBuffer* pingpong1_framebuffer = context.frameBufferOfTarget(RGTarget::BloomPingPong1);
    RhiFrameBuffer* pingpong2_framebuffer = context.frameBufferOfTarget(RGTarget::BloomPingPong2);
    RhiTexture* bright_texture = context.texture(RGResource::BloomBrightColor);
    RhiTexture* pingpong1_texture = context.texture(RGResource::BloomPingPong1Color);
    RhiTexture* pingpong2_texture = context.texture(RGResource::BloomPingPong2Color);
    if (!pingpong1_framebuffer || !pingpong2_framebuffer ||
        !bright_texture || !pingpong1_texture || !pingpong2_texture)
        return;

    m_command_buffer->beginPass(pingpong1_framebuffer);
    m_command_buffer->endPass();
    m_command_buffer->beginPass(pingpong2_framebuffer);
    m_command_buffer->endPass();

    bool horizontal = true;
    unsigned int amount = 16;
    for (unsigned int i = 0; i < amount; i++)
    {
        RhiFrameBuffer* target = horizontal ? pingpong1_framebuffer : pingpong2_framebuffer;
        m_command_buffer->beginPass(target, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);
        m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::GaussianBlur));
        RhiTexture* source_texture = (i == 0) ? bright_texture :
            (horizontal ? pingpong2_texture : pingpong1_texture);
        ShaderResourceBindings bindings;
        bindings.setTexture("image", 0, source_texture);
        bindings.setInt("horizontal", horizontal);
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(context.builtinResources().screen_quad->vertexLayout());
        m_command_buffer->drawIndexed(static_cast<int>(context.builtinResources().screen_quad->indicesCount()));
        m_command_buffer->endPass();
        horizontal = !horizontal;
    }
}

void BloomPass::writeToScene(RenderPassContext& context)
{
    RhiFrameBuffer* scene_framebuffer = context.frameBuffer(RGResource::SceneColor);
    m_command_buffer->beginPass(scene_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    RenderPipelineState state;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::BloomShader, state));
    ShaderResourceBindings bindings;
    bindings.setTexture("Texture", 0, context.texture(RGResource::SceneColor));
    bindings.setTexture("bloomMap", 1, context.texture(RGResource::BloomPingPong1Color));
    m_command_buffer->setShaderResources(&bindings);
    m_command_buffer->setVertexInput(context.builtinResources().screen_quad->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(context.builtinResources().screen_quad->indicesCount()));
    m_command_buffer->endPass();
}
