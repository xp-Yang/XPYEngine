#include "FXAAPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

FXAAPass::FXAAPass()
{
	m_type = RenderPass::Type::FXAA;
}

void FXAAPass::draw(RenderPassContext& context)
{
	m_rhi->setDepthMask(false);

    // TODO 渲染目标纹理同时作为采样纹理，结果是未定义的
    // BloomPass里也存在绑定SceneColor作为输出同时读SceneColor的行为
	RhiFrameBuffer* framebuffer = context.frameBuffer(RGResource::SceneColor);
    framebuffer->bind();
    static RenderShaderObject* fxaa_shader = RenderShaderObject::getShaderObject(ShaderType::FXAAShader);
    fxaa_shader->start_using();
    auto color_map = framebuffer->colorAttachmentAt(0)->texture()->id();
    fxaa_shader->setTexture("mainTexture", 0, color_map);
    m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());

    m_rhi->setDepthMask(true);
}
