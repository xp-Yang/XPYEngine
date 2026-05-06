#include "DeferredLightingPass.hpp"

DeferredLightingPass::DeferredLightingPass()
{
	m_type = RenderPass::Type::DeferredLighting;
	init();
}

void DeferredLightingPass::init()
{
	rebuildFramebuffer(Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
}

void DeferredLightingPass::rebuildFramebuffer(const Vec2 &pixel_size)
{
	RhiTexture *color_texture = m_rhi->newTexture(RhiTexture::Format::RGBA16F, pixel_size);
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

void DeferredLightingPass::rebuildFramebuffers(const Vec2 &pixel_size)
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

void DeferredLightingPass::draw()
{
	m_framebuffer->bind();
	//m_framebuffer->clear(Color4(0.046, 0.046, 0.046, 1.0)); // before gamma correction
	m_framebuffer->clear(Color4(0.251, 0.251, 0.251, 1.0)); // after gamma correction

	// deferred lighting
	static RenderShaderObject* lighting_pbr_shader = RenderShaderObject::getShaderObject(ShaderType::DeferredLightingShader);
	static RenderShaderObject* lighting_phong_shader = RenderShaderObject::getShaderObject(ShaderType::DeferredLightingPhongShader);
	RenderShaderObject* lighting_shader = m_pbr ? lighting_pbr_shader : lighting_phong_shader;
	RhiFrameBuffer* gbuffer_framebuffer = m_input_passes[0]->getFrameBuffer();
	lighting_shader->start_using();
	unsigned int g_position_map = gbuffer_framebuffer->colorAttachmentAt(0)->texture()->id();
	unsigned int g_normal_map = gbuffer_framebuffer->colorAttachmentAt(1)->texture()->id();
	lighting_shader->setTexture("gPosition", 0, g_position_map);
	lighting_shader->setTexture("gNormal", 1, g_normal_map);
	if (m_pbr) {
		unsigned int g_albedo_map = gbuffer_framebuffer->colorAttachmentAt(2)->texture()->id();
		unsigned int g_metallic_map = gbuffer_framebuffer->colorAttachmentAt(3)->texture()->id();
		unsigned int g_roughness_map = gbuffer_framebuffer->colorAttachmentAt(4)->texture()->id();
		unsigned int g_ao_map = gbuffer_framebuffer->colorAttachmentAt(5)->texture()->id();
		lighting_shader->setTexture("gAlbedo", 2, g_albedo_map);
		lighting_shader->setTexture("gMetallic", 3, g_metallic_map);
		lighting_shader->setTexture("gRoughness", 4, g_roughness_map);
		lighting_shader->setTexture("gAo", 5, g_ao_map);
	}
	else {
		unsigned int g_diffuse_map = gbuffer_framebuffer->colorAttachmentAt(2)->texture()->id();
		unsigned int g_specular_map = gbuffer_framebuffer->colorAttachmentAt(3)->texture()->id();
		lighting_shader->setTexture("gDiffuse", 2, g_diffuse_map);
		lighting_shader->setTexture("gSpecular", 3, g_specular_map);
	}

	RhiFrameBuffer* shadow_framebuffer = m_input_passes[1]->getFrameBuffer();
	m_dir_light_shadow_map = shadow_framebuffer->depthAttachment()->texture()->id();
	lighting_shader->setFloat3("cameraPos", m_render_source_data->camera_position);
	for (const auto& render_directional_light_data : m_render_source_data->render_directional_light_data_list) {
		lighting_shader->setFloat3("directionalLight.direction", render_directional_light_data.direction);
		lighting_shader->setFloat4("directionalLight.color", render_directional_light_data.color);
		if (m_dir_light_shadow_map != 0) {
			lighting_shader->setMatrix("lightSpaceMatrix", 1, render_directional_light_data.lightProjMatrix * render_directional_light_data.lightViewMatrix);
			lighting_shader->setTexture("shadow_map", 8, m_dir_light_shadow_map);
		}
	}
	for (int i = 0; i < m_cube_maps.size(); i++) {
		std::string cube_map_id = std::string("cube_shadow_maps[") + std::to_string(i) + "]";
		lighting_shader->setCubeTexture(cube_map_id, 9 + i, m_cube_maps[i]);
	}
	int point_light_idx = 0;
	for (const auto& render_point_light_data : m_render_source_data->render_point_light_data_list) {
		std::string light_id = std::string("pointLights[") + std::to_string(point_light_idx) + "]";
		lighting_shader->setFloat3(light_id + ".position", render_point_light_data.position);
		lighting_shader->setFloat4(light_id + ".color", render_point_light_data.color);
		lighting_shader->setFloat(light_id + ".radius", render_point_light_data.radius);
		point_light_idx++;
	}
	lighting_shader->setInt("point_lights_size", m_render_source_data->render_point_light_data_list.size());
	m_rhi->drawIndexed(m_render_source_data->screen_quad->getVAO(), m_render_source_data->screen_quad->indicesCount());
	lighting_shader->stop_using();


	gbuffer_framebuffer->blitTo(m_framebuffer.get(), RhiTexture::Format::DEPTH);

	// lights
	//static RenderShaderObject* point_light_shader = RenderShaderObject::getShaderObject(ShaderType::OneColorShader);
	//for (const auto& render_point_light_data : m_render_source_data->render_point_light_data_list) {
	//	const auto& render_point_light_sub_mesh_data = render_point_light_data.render_sub_mesh_data;
	//	point_light_shader->start_using();
	//	point_light_shader->setFloat4("color", render_point_light_data.color);
	//	point_light_shader->setMatrix("model", 1, render_point_light_sub_mesh_data->transform());
	//	point_light_shader->setMatrix("view", 1, m_render_source_data->view_matrix);
	//	point_light_shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
	//	m_rhi->drawIndexed(render_point_light_sub_mesh_data->getVAO(), render_point_light_sub_mesh_data->indicesCount());
	//	point_light_shader->stop_using();
	//}

	// instancing lights
    if (m_render_source_data->render_point_light_inst_mesh && m_render_source_data->point_light_inst_amount > 0)
    {
        static RenderShaderObject* point_light_instancing_shader = RenderShaderObject::getShaderObject(ShaderType::InstancingShader);
        point_light_instancing_shader->start_using();
        point_light_instancing_shader->setMatrix("view", 1, m_render_source_data->view_matrix);
        point_light_instancing_shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
        m_rhi->drawIndexed(
            m_render_source_data->render_point_light_inst_mesh->getVAO(),
            m_render_source_data->render_point_light_inst_mesh->indicesCount(),
            0,
            m_render_source_data->point_light_inst_amount);
        point_light_instancing_shader->stop_using();
    }

	m_framebuffer->unBind();
}

void DeferredLightingPass::enablePBR(bool enable)
{
	m_pbr = enable;
}
