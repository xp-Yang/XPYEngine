#include "RayTracingPass.hpp"

RayTracingPass::RayTracingPass()
{
	init();
}

void RayTracingPass::init()
{
	rebuildFramebuffer(Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
}

void RayTracingPass::rebuildFramebuffer(const Vec2 &pixel_size)
{
	RhiTexture *color_texture = m_rhi->newTexture(RhiTexture::Format::RGB16F, pixel_size);
	RhiTexture *depth_texture = m_rhi->newTexture(RhiTexture::Format::DEPTH, pixel_size);
	color_texture->create();
	depth_texture->create();
	RhiAttachment color_attachment = RhiAttachment(color_texture);
	RhiAttachment depth_ttachment = RhiAttachment(depth_texture);
	RhiFrameBuffer *fb = m_rhi->newFrameBuffer(color_attachment, pixel_size);
	fb->setDepthAttachment(depth_ttachment);
	fb->create();
	m_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);
}

void RayTracingPass::rebuildFramebuffers(const Vec2 &pixel_size)
{
	Vec2 sz = clampFramebufferPixelSize(pixel_size);
	if (m_framebuffer && (int)m_framebuffer->pixelSize().x == (int)sz.x && (int)m_framebuffer->pixelSize().y == (int)sz.y)
		return;
	if (m_framebuffer)
	{
		m_framebuffer->destroyGPU();
		m_framebuffer.reset();
	}
	rebuildFramebuffer(sz);
}

void RayTracingPass::draw()
{
	m_framebuffer->bind();
	m_framebuffer->clear();

	static RenderShaderObject *rt_shader = RenderShaderObject::getShaderObject(ShaderType::RayTracingShader);
	auto camera = m_render_source_data->render_camera;

	{
		rt_shader->start_using();
		// camera
		rt_shader->setFloat3("camera.pos", camera->pos);
		rt_shader->setFloat("camera.distance", 1.0f /*camera->focal_length*/);
		rt_shader->setFloat("camera.fov", camera->fov);
		rt_shader->setFloat("camera.aspect_ratio", 16.0f / 9.0f); // TODO 需要一种方法不会拉伸纹理也不影响fov

		rt_shader->setFloat3("camera.front", camera->direction);
		rt_shader->setFloat3("camera.right", camera->rightDirection);
		rt_shader->setFloat3("camera.up", camera->upDirection);

		// debug
		// float width = tan(camera->fov / 2.0f) * camera->focal_length * 2.0;
		// float height = width / (16.0f / 9.0f);
		// Vec3 origin = camera->pos + camera->focal_length * camera->direction;
		// Vec3 leftbottom = camera->pos + camera->focal_length * camera->direction - width / 2.0f * camera->getRightDirection() - height / 2.0f * camera->upDirection;

		// random
		rt_shader->setFloat("randOrigin", 674764.0f * (Math::randomUnit() + 1.0f));
		// render to m_framebuffer
		m_rhi->drawIndexed(m_render_source_data->screen_quad->getVAO(), m_render_source_data->screen_quad->indicesCount());
	}

	m_framebuffer->unBind();
}
