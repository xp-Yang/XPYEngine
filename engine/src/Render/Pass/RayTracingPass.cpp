#include "RayTracingPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

RayTracingPass::RayTracingPass()
{
	m_type = RenderPass::Type::RayTracing;
}

void RayTracingPass::draw(RenderPassContext &context)
{
	RhiFrameBuffer *framebuffer = context.frameBufferOfTarget(slot("outTarget"));
	if (!framebuffer)
		return;
	m_command_buffer->beginPass(framebuffer);

	// auto camera = context.frameData().render_camera;

	{
		// m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::RayTracingShader));
		// ShaderResourceBindings bindings;
		// // camera
		// bindings.setFloat3("camera.pos", camera->pos);
		// bindings.setFloat("camera.distance", 1.0f /*camera->focal_length*/);
		// bindings.setFloat("camera.fov", camera->fov);
		// bindings.setFloat("camera.aspect_ratio", 16.0f / 9.0f); // TODO 需要一种方法不会拉伸纹理也不影响fov

		// bindings.setFloat3("camera.front", camera->direction);
		// bindings.setFloat3("camera.right", camera->rightDirection);
		// bindings.setFloat3("camera.up", camera->upDirection);

		// // debug
		// // float width = tan(camera->fov / 2.0f) * camera->focal_length * 2.0;
		// // float height = width / (16.0f / 9.0f);
		// // Vec3 origin = camera->pos + camera->focal_length * camera->direction;
		// // Vec3 leftbottom = camera->pos + camera->focal_length * camera->direction - width / 2.0f * camera->getRightDirection() - height / 2.0f * camera->upDirection;

		// // random
		// bindings.setFloat("randOrigin", 674764.0f * (Math::randomUnit() + 1.0f));
		// m_command_buffer->setShaderResources(&bindings);
		// // render to framebuffer
		// m_command_buffer->setVertexInput(context.builtinResources().screen_quad->vertexLayout());
		// m_command_buffer->drawIndexed(static_cast<int>(context.builtinResources().screen_quad->indicesCount()));
	}

	m_command_buffer->endPass();
}
