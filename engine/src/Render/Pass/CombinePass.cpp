#include "CombinePass.hpp"

CombinePass::CombinePass()
{
	m_type = RenderPass::Type::Combined;
	init();
}

void CombinePass::init()
{
	RhiTexture* color_texture = m_rhi->newTexture(RhiTexture::Format::RGB8, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
	RhiTexture* depth_texture = m_rhi->newTexture(RhiTexture::Format::DEPTH, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
	color_texture->create();
	depth_texture->create();
	RhiAttachment color_attachment = RhiAttachment(color_texture);
	RhiAttachment depth_ttachment = RhiAttachment(depth_texture);
	RhiFrameBuffer* fb = m_rhi->newFrameBuffer(color_attachment, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
	fb->setDepthAttachment(depth_ttachment);
	fb->create();
	m_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);

	RhiFrameBuffer* default_fb = m_rhi->newFrameBuffer(RhiAttachment(), Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
	m_default_framebuffer = std::unique_ptr<RhiFrameBuffer>(default_fb);
}

void CombinePass::draw()
{
	m_framebuffer->bind();
	m_framebuffer->clear();

	m_input_passes[0]->getFrameBuffer()->blitTo(m_framebuffer.get(), RhiTexture::Format::RGB8); //downSample if msaa
	m_input_passes[0]->getFrameBuffer()->blitTo(m_framebuffer.get(), RhiTexture::Format::DEPTH);

	m_rhi->setDepthMask(false);

	// post processing
	static RenderShaderObject* combine_shader = RenderShaderObject::getShaderObject(ShaderType::CombineShader);
	unsigned int default_map = RenderTextureData::defaultTexture().id;
	combine_shader->start_using();
	auto lighted_map = m_framebuffer->colorAttachmentAt(0)->texture()->id();
	combine_shader->setTexture("Texture", 0, lighted_map);
	if (m_input_passes.size() > 1) {
		auto blurred_bright_map = m_input_passes[1]->getFrameBuffer()->colorAttachmentAt(0)->texture()->id();
		combine_shader->setTexture("bloomMap", 1, blurred_bright_map);
	}
	else
		combine_shader->setTexture("bloomMap", 1, default_map);
	m_rhi->drawIndexed(m_render_source_data->screen_quad->getVAO(), m_render_source_data->screen_quad->indicesCount());

	
	// fxaa
	if (m_fxaa) {
		static RenderShaderObject* fxaa_shader = RenderShaderObject::getShaderObject(ShaderType::FXAAShader);
		fxaa_shader->start_using();
		auto color_map = m_framebuffer->colorAttachmentAt(0)->texture()->id();
		fxaa_shader->setTexture("mainTexture", 0, color_map);
		m_rhi->drawIndexed(m_render_source_data->screen_quad->getVAO(), m_render_source_data->screen_quad->indicesCount());
	}

	// pristine grid
	static RenderShaderObject* grid_shader = RenderShaderObject::getShaderObject(ShaderType::PristineGridShader);
	grid_shader->start_using();
	grid_shader->setMatrix("view", 1, m_render_source_data->view_matrix);
	grid_shader->setMatrix("proj", 1, m_render_source_data->proj_matrix);
	m_rhi->drawIndexed(m_render_source_data->screen_quad->getVAO(), m_render_source_data->screen_quad->indicesCount());
	grid_shader->stop_using();

	m_rhi->setDepthMask(true);

	m_default_framebuffer->bind();
	m_default_framebuffer->clear(Color4(0.45f, 0.55f, 0.60f, 1.00f));
}

void CombinePass::enableFXAA(bool enable)
{
	m_fxaa = enable;
}
