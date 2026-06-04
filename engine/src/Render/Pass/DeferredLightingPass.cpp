#include "DeferredLightingPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

DeferredLightingPass::DeferredLightingPass()
{
	m_type = RenderPass::Type::DeferredLighting;
}

void DeferredLightingPass::draw(RenderPassContext& context)
{
	RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(slot("outTarget"));
	RhiFrameBuffer* gbuffer_framebuffer = context.frameBuffer(slot("inGBufferDepthFBO"));
	if (!framebuffer || !gbuffer_framebuffer)
		return;
	m_command_buffer->beginPass(framebuffer, Color4(0.046f, 0.046f, 0.046f, 1.0f));

	const ShaderType lighting_shader = m_pbr ? ShaderType::DeferredLightingShader : ShaderType::DeferredLightingPhongShader;
	m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(lighting_shader));
    ShaderResourceBindings bindings;
	RhiTexture* g_position_map = context.texture(slot("inGBufferPosition"));
	RhiTexture* g_normal_map = context.texture(slot("inGBufferNormal"));
	if (!g_position_map || !g_normal_map)
	{
		m_command_buffer->endPass();
		return;
	}
	bindings.setTexture("gPosition", 0, g_position_map);
	bindings.setTexture("gNormal", 1, g_normal_map);
	if (m_pbr) {
		RhiTexture* g_albedo_map = context.texture(slot("inGBufferAlbedo"));
		RhiTexture* g_metallic_map = context.texture(slot("inGBufferMetallic"));
		RhiTexture* g_roughness_map = context.texture(slot("inGBufferRoughness"));
		RhiTexture* g_ao_map = context.texture(slot("inGBufferAO"));
		if (!g_albedo_map || !g_metallic_map || !g_roughness_map || !g_ao_map)
		{
			m_command_buffer->endPass();
			return;
		}
		bindings.setTexture("gAlbedo", 2, g_albedo_map);
		bindings.setTexture("gMetallic", 3, g_metallic_map);
		bindings.setTexture("gRoughness", 4, g_roughness_map);
		bindings.setTexture("gAo", 5, g_ao_map);
	}
	else {
		RhiTexture* g_diffuse_map = context.texture(slot("inGBufferDiffuse"));
		RhiTexture* g_specular_map = context.texture(slot("inGBufferSpecular"));
		if (!g_diffuse_map || !g_specular_map)
		{
			m_command_buffer->endPass();
			return;
		}
		bindings.setTexture("gDiffuse", 2, g_diffuse_map);
		bindings.setTexture("gSpecular", 3, g_specular_map);
	}

	RhiTexture* dir_light_shadow_texture = context.texture(slot("inShadowDepth"));
	bindings.setBool("directionalShadowEnable", m_directional_shadow && dir_light_shadow_texture != nullptr);
	bindings.setFloat3("cameraPos", context.frameData().camera_position);
	for (const auto& render_directional_light_data : context.frameData().directional_lights) {
		bindings.setFloat3("directionalLight.direction", render_directional_light_data.direction);
		bindings.setFloat3("directionalLight.color", render_directional_light_data.color);
		if (dir_light_shadow_texture) {
			bindings.setMatrix("lightSpaceMatrix", 1, render_directional_light_data.lightProjMatrix * render_directional_light_data.lightViewMatrix);
			bindings.setTexture("shadow_map", 6, dir_light_shadow_texture);
		}
    }
		// IBL 环境光（固定单元 7/8/9，见 IBL 方案“纹理单元预算”方案 A）。
		const IBLResources& ibl = context.builtinResources().ibl;
		const bool ibl_ready = m_pbr && m_ibl && ibl.isReady();
		bindings.setBool("iblEnable", ibl_ready);
		RhiTexture* default_cube = RenderTextureResource::defaultCubeTexture()->texture();
		RhiTexture* default_2d = RenderTextureResource::defaultTexture()->texture();
		bindings.setCubeTexture("irradianceMap", 7, ibl_ready ? ibl.irradiance_cube : default_cube);
		bindings.setCubeTexture("prefilterMap", 8, ibl_ready ? ibl.prefilter_cube : default_cube);
		bindings.setTexture("brdfLUT", 9, ibl_ready ? ibl.brdf_lut : default_2d);

    std::vector<RhiTexture*> cube_shadow_maps = context.cubeShadowMaps();
    const int max_cube_shadow_maps = static_cast<int>(MAX_CUBE_SHADOW_MAP_COUNT);
    for (int i = 0; i < max_cube_shadow_maps; i++) {
        RhiTexture* cube_shadow_map =
            i < static_cast<int>(cube_shadow_maps.size()) && cube_shadow_maps[i]
            ? cube_shadow_maps[i]
            : RenderTextureResource::defaultCubeTexture()->texture();
        std::string cube_map_id_str = std::string("cube_shadow_maps[") + std::to_string(i) + "]";
        bindings.setCubeTexture(cube_map_id_str, 10 + i, cube_shadow_map);
    }

    const int point_light_size = std::min(max_cube_shadow_maps, static_cast<int>(context.frameData().point_lights.size()));
    bool has_point_shadow_slot = false;
	int point_light_idx = 0;
	for (const auto& render_point_light_data : context.frameData().point_lights) {
        if (point_light_idx >= point_light_size)
            break;
		std::string light_id = std::string("pointLights[") + std::to_string(point_light_idx) + "]";
		bindings.setFloat3(light_id + ".position", render_point_light_data.position);
		bindings.setFloat3(light_id + ".color", render_point_light_data.color);
		bindings.setFloat(light_id + ".radius", render_point_light_data.radius);
		bindings.setInt(light_id + ".shadowIndex", render_point_light_data.shadow_index);
        has_point_shadow_slot = has_point_shadow_slot ||
            (render_point_light_data.shadow_index >= 0 &&
             render_point_light_data.shadow_index < static_cast<int>(cube_shadow_maps.size()) &&
             cube_shadow_maps[render_point_light_data.shadow_index]);
		point_light_idx++;
	}
	bindings.setInt("point_lights_size", point_light_size);
	bindings.setBool("pointShadowEnable", m_point_shadow && has_point_shadow_slot);

	// SSAO（单元 15，关闭时绑定默认白图并由 ssaoEnable 门控）。
	const bool ssao_ready = m_pbr && m_ssao;
	bindings.setBool("ssaoEnable", ssao_ready);
	RhiTexture* ssao_map = (ssao_ready && hasSlot("inSSAO")) ? context.texture(slot("inSSAO")) : nullptr;
	bindings.setTexture("ssaoMap", 15, ssao_map ? ssao_map : RenderTextureResource::defaultTexture()->texture());

    m_command_buffer->setShaderResources(&bindings);
	m_command_buffer->setVertexInput(context.builtinResources().screen_quad->vertexLayout());
	m_command_buffer->drawIndexed(static_cast<int>(context.builtinResources().screen_quad->indicesCount()));
	m_command_buffer->endPass();

	m_command_buffer->blit(gbuffer_framebuffer, framebuffer, RhiTexture::Format::DEPTH);

	// instancing lights
    if (context.builtinResources().point_light_inst_mesh && context.frameData().point_light_inst_amount > 0)
    {
        m_command_buffer->beginPass(framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);
        m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::InstancingShader));
        ShaderResourceBindings instancing_bindings;
        instancing_bindings.setMatrix("view", 1, context.frameData().view_matrix);
        instancing_bindings.setMatrix("projection", 1, context.frameData().proj_matrix);
        m_command_buffer->setShaderResources(&instancing_bindings);
        m_command_buffer->setVertexInput(context.builtinResources().point_light_inst_mesh->vertexLayout());
        m_command_buffer->drawIndexed(
            static_cast<int>(context.builtinResources().point_light_inst_mesh->indicesCount()),
            context.frameData().point_light_inst_amount);
        m_command_buffer->endPass();
    }
}

void DeferredLightingPass::enablePBR(bool enable)
{
	m_pbr = enable;
}
