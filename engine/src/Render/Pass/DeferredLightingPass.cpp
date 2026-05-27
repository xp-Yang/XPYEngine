#include "DeferredLightingPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

DeferredLightingPass::DeferredLightingPass()
{
	m_type = RenderPass::Type::DeferredLighting;
}

void DeferredLightingPass::draw(RenderPassContext& context)
{
	RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);
	RhiFrameBuffer* gbuffer_framebuffer = context.frameBuffer(RGResource::GBufferPosition);
	if (!framebuffer || !gbuffer_framebuffer)
		return;
	// framebuffer clear color before gamma correction: Color4(0.046, 0.046, 0.046, 1.0)
	m_command_buffer->beginPass(framebuffer, Color4(0.251, 0.251, 0.251, 1.0)); // after gamma correction

	// deferred lighting
    const ShaderType lighting_shader = m_pbr ? ShaderType::DeferredLightingShader : ShaderType::DeferredLightingPhongShader;
	m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(lighting_shader));
    ShaderResourceBindings bindings;
	GL_HANDLE g_position_map = gbuffer_framebuffer->colorAttachmentAt(0)->texture()->id();
	GL_HANDLE g_normal_map = gbuffer_framebuffer->colorAttachmentAt(1)->texture()->id();
	bindings.setTexture("gPosition", 0, g_position_map);
	bindings.setTexture("gNormal", 1, g_normal_map);
	if (m_pbr) {
		GL_HANDLE g_albedo_map = gbuffer_framebuffer->colorAttachmentAt(2)->texture()->id();
		GL_HANDLE g_metallic_map = gbuffer_framebuffer->colorAttachmentAt(3)->texture()->id();
		GL_HANDLE g_roughness_map = gbuffer_framebuffer->colorAttachmentAt(4)->texture()->id();
		GL_HANDLE g_ao_map = gbuffer_framebuffer->colorAttachmentAt(5)->texture()->id();
		bindings.setTexture("gAlbedo", 2, g_albedo_map);
		bindings.setTexture("gMetallic", 3, g_metallic_map);
		bindings.setTexture("gRoughness", 4, g_roughness_map);
		bindings.setTexture("gAo", 5, g_ao_map);
	}
	else {
		GL_HANDLE g_diffuse_map = gbuffer_framebuffer->colorAttachmentAt(2)->texture()->id();
		GL_HANDLE g_specular_map = gbuffer_framebuffer->colorAttachmentAt(3)->texture()->id();
		bindings.setTexture("gDiffuse", 2, g_diffuse_map);
		bindings.setTexture("gSpecular", 3, g_specular_map);
	}

	RhiTexture* dir_light_shadow_texture = context.texture(RGResource::ShadowDirectionalDepth);
	bindings.setFloat3("cameraPos", context.renderSourceData().camera_position);
	for (const auto& render_directional_light_data : context.renderSourceData().render_directional_light_data_list) {
		bindings.setFloat3("directionalLight.direction", render_directional_light_data.direction);
		bindings.setFloat3("directionalLight.color", render_directional_light_data.color);
		if (dir_light_shadow_texture) {
			bindings.setMatrix("lightSpaceMatrix", 1, render_directional_light_data.lightProjMatrix * render_directional_light_data.lightViewMatrix);
			bindings.setTexture("shadow_map", 6, dir_light_shadow_texture->id());
		}
	}
    std::vector<RhiTexture*> cube_shadow_maps = context.cubeShadowMaps();
    int point_light_size = std::min(MAX_CUBE_SHADOW_MAP_COUNT, cube_shadow_maps.size());
    for (int i = 0; i < MAX_CUBE_SHADOW_MAP_COUNT; i++) {
        GL_HANDLE cube_shadow_map_id =
            i < static_cast<int>(cube_shadow_maps.size()) && cube_shadow_maps[i]
            ? cube_shadow_maps[i]->id()
            : RenderTextureData::defaultCubeTexture().id;
        std::string cube_map_id_str = std::string("cube_shadow_maps[") + std::to_string(i) + "]";
        bindings.setCubeTexture(cube_map_id_str, 7 + i, cube_shadow_map_id);
    }

	int point_light_idx = 0;
	for (const auto& render_point_light_data : context.renderSourceData().render_point_light_data_list) {
		std::string light_id = std::string("pointLights[") + std::to_string(point_light_idx) + "]";
		bindings.setFloat3(light_id + ".position", render_point_light_data.position);
		bindings.setFloat3(light_id + ".color", render_point_light_data.color);
		bindings.setFloat(light_id + ".radius", render_point_light_data.radius);
		point_light_idx++;
	}
	bindings.setInt("point_lights_size", point_light_size);
    m_command_buffer->setShaderResources(&bindings);
	m_command_buffer->setVertexInput(context.renderSourceData().screen_quad->vertexLayout());
	m_command_buffer->drawIndexed(static_cast<int>(context.renderSourceData().screen_quad->indicesCount()));
	m_command_buffer->endPass();

	m_command_buffer->blit(gbuffer_framebuffer, framebuffer, RhiTexture::Format::DEPTH);

	// instancing lights
    if (context.renderSourceData().render_point_light_inst_mesh && context.renderSourceData().point_light_inst_amount > 0)
    {
        m_command_buffer->beginPass(framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);
        m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::InstancingShader));
        ShaderResourceBindings instancing_bindings;
        instancing_bindings.setMatrix("view", 1, context.renderSourceData().view_matrix);
        instancing_bindings.setMatrix("projection", 1, context.renderSourceData().proj_matrix);
        m_command_buffer->setShaderResources(&instancing_bindings);
        m_command_buffer->setVertexInput(context.renderSourceData().render_point_light_inst_mesh->vertexLayout());
        m_command_buffer->drawIndexed(
            static_cast<int>(context.renderSourceData().render_point_light_inst_mesh->indicesCount()),
            context.renderSourceData().point_light_inst_amount);
        m_command_buffer->endPass();
    }
}

void DeferredLightingPass::enablePBR(bool enable)
{
	m_pbr = enable;
}
