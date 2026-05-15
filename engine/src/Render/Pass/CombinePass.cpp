#include "CombinePass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

CombinePass::CombinePass()
{
	m_type = RenderPass::Type::Combined;
}

void CombinePass::draw(RenderPassContext& context)
{
	RhiFrameBuffer* framebuffer = context.targetFrameBuffer();
	RhiFrameBuffer* default_framebuffer = context.defaultFrameBuffer();
	RhiFrameBuffer* source_framebuffer = context.readFrameBuffer(RGSlot::Source);
	if (!framebuffer || !default_framebuffer || !source_framebuffer)
		return;

	framebuffer->bind();
	framebuffer->clear();

	source_framebuffer->blitTo(framebuffer, RhiTexture::Format::RGB8); //downSample if msaa
	source_framebuffer->blitTo(framebuffer, RhiTexture::Format::DEPTH);

	m_rhi->setDepthMask(false);

	// post processing
	static RenderShaderObject* combine_shader = RenderShaderObject::getShaderObject(ShaderType::CombineShader);
	unsigned int default_map = RenderTextureData::defaultTexture().id;
	combine_shader->start_using();
	auto lighted_map = framebuffer->colorAttachmentAt(0)->texture()->id();
	combine_shader->setTexture("Texture", 0, lighted_map);
	RhiTexture* blurred_bright_texture = context.readTexture(RGSlot::Bloom);
	if (blurred_bright_texture) {
		auto blurred_bright_map = blurred_bright_texture->id();
		combine_shader->setTexture("bloomMap", 1, blurred_bright_map);
	}
	else
		combine_shader->setTexture("bloomMap", 1, default_map);
	m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());

	
	// fxaa
	if (m_fxaa) {
		static RenderShaderObject* fxaa_shader = RenderShaderObject::getShaderObject(ShaderType::FXAAShader);
		fxaa_shader->start_using();
		auto color_map = framebuffer->colorAttachmentAt(0)->texture()->id();
		fxaa_shader->setTexture("mainTexture", 0, color_map);
		m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());
	}

	// pristine grid
	static RenderShaderObject* grid_shader = RenderShaderObject::getShaderObject(ShaderType::PristineGridShader);
	grid_shader->start_using();
	grid_shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
	grid_shader->setMatrix("proj", 1, context.renderSourceData().proj_matrix);
	m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());
	grid_shader->stop_using();

	m_rhi->setDepthMask(true);

	default_framebuffer->bind();
	default_framebuffer->clear(Color4(0.45f, 0.55f, 0.60f, 1.00f));
}

void CombinePass::enableFXAA(bool enable)
{
	m_fxaa = enable;
}
