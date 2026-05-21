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
    m_command_buffer->beginPass(framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);
    static RenderShaderObject* fxaa_shader = RenderShaderObject::getShaderObject(ShaderType::FXAAShader);
    RenderPipelineState state;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(fxaa_shader->graphicsPipeline(state));
    auto color_map = framebuffer->colorAttachmentAt(0)->texture()->id();
    fxaa_shader->setTexture("mainTexture", 0, color_map);
    m_command_buffer->setVertexInput(context.renderSourceData().screen_quad->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(context.renderSourceData().screen_quad->indicesCount()));
    m_command_buffer->endPass();
}
