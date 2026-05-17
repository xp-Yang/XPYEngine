#include "FinalPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

FinalPass::FinalPass()
{
	m_type = RenderPass::Type::Final;
}

void FinalPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);

    framebuffer->bind();

    RhiFrameBuffer* source_framebuffer = nullptr;
    source_framebuffer = context.frameBuffer(RGResource::CheckerBoardColor);
        if (!source_framebuffer)
            source_framebuffer = context.frameBuffer(RGResource::SceneColor);
    // TODO //downSample if msaa?
    source_framebuffer->blitTo(framebuffer, RhiTexture::Format::RGB16F);
    source_framebuffer->blitTo(framebuffer, RhiTexture::Format::DEPTH);

    m_rhi->setBlend(true);
    m_rhi->setDepthMask(false);

    // pristine grid
    static RenderShaderObject* grid_shader = RenderShaderObject::getShaderObject(ShaderType::PristineGridShader);
    grid_shader->start_using();
    grid_shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
    grid_shader->setMatrix("proj", 1, context.renderSourceData().proj_matrix);
    m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());
    grid_shader->stop_using();

    m_rhi->setBlend(false);
    m_rhi->setDepthMask(true);


    RhiFrameBuffer* default_framebuffer = context.defaultFrameBuffer();
    default_framebuffer->bind();
    default_framebuffer->clear(Color4(0.45f, 0.55f, 0.60f, 1.00f));
}
