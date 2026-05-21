#include "FinalPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

FinalPass::FinalPass()
{
	m_type = RenderPass::Type::Final;
}

void FinalPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);

    RhiFrameBuffer* source_framebuffer = nullptr;
    source_framebuffer = context.frameBuffer(RGResource::CheckerBoardColor);
        if (!source_framebuffer)
            source_framebuffer = context.frameBuffer(RGResource::SceneColor);
    // TODO //downSample if msaa?
    m_command_buffer->blit(source_framebuffer, framebuffer, RhiTexture::Format::RGB16F);
    m_command_buffer->blit(source_framebuffer, framebuffer, RhiTexture::Format::DEPTH);
    m_command_buffer->beginPass(framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    // pristine grid
    static RenderShaderObject* grid_shader = RenderShaderObject::getShaderObject(ShaderType::PristineGridShader);
    RenderPipelineState state;
    state.blend = true;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(grid_shader->graphicsPipeline(state));
    grid_shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
    grid_shader->setMatrix("proj", 1, context.renderSourceData().proj_matrix);
    m_command_buffer->setVertexInput(context.renderSourceData().screen_quad->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(context.renderSourceData().screen_quad->indicesCount()));
    m_command_buffer->endPass();


    RhiFrameBuffer* default_framebuffer = context.defaultFrameBuffer();
    m_command_buffer->beginPass(default_framebuffer, Color4(0.45f, 0.55f, 0.60f, 1.00f));
    m_command_buffer->endPass();
}
