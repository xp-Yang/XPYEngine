#include "RayTracingPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

RayTracingPass::RayTracingPass()
{
	m_type = RenderPass::Type::RayTracing;
}

void RayTracingPass::draw(RenderPassContext& context)
{
	RhiFrameBuffer* framebuffer = context.targetFrameBuffer();
	if (!framebuffer)
		return;
	framebuffer->bind();
	framebuffer->clear();

	static RenderShaderObject *rt_shader = RenderShaderObject::getShaderObject(ShaderType::RayTracingShader);
	auto camera = context.renderSourceData().render_camera;

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
		// render to framebuffer
		m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());
	}

	framebuffer->unBind();
}
