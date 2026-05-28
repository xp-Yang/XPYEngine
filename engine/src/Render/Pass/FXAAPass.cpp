#include "FXAAPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

FXAAPass::FXAAPass()
{
	m_type = RenderPass::Type::FXAA;
}

void FXAAPass::draw(RenderPassContext& context)
{
    // TODO 渲染目标纹理同时作为采样纹理，结果是未定义的
    // BloomPass里也存在绑定SceneColor作为输出同时读SceneColor的行为
	RhiFrameBuffer* framebuffer = context.frameBuffer(RGResource::SceneColor);
    RhiTexture* color_map = context.texture(RGResource::SceneColor);
    if (!framebuffer || !color_map)
        return;
    m_command_buffer->beginPass(framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);
    RenderPipelineState state;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::FXAAShader, state));
    ShaderResourceBindings bindings;
    bindings.setTexture("mainTexture", 0, color_map);
    m_command_buffer->setShaderResources(&bindings);
    m_command_buffer->setVertexInput(context.builtinResources().screen_quad->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(context.builtinResources().screen_quad->indicesCount()));
    m_command_buffer->endPass();
}
