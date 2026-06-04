#include "MeshForwardLightingPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

MeshForwardLightingPass::MeshForwardLightingPass()
{
    m_type = RenderPass::Type::Forward;
}

void MeshForwardLightingPass::enableReflection(bool reflection)
{
    // config shader ²ÎÊý
    m_reflection = reflection;
}

void MeshForwardLightingPass::enablePBR(bool pbr)
{
    m_pbr = pbr;
}

void MeshForwardLightingPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(slot("outTarget"));
    if (!framebuffer)
        return;
    // framebuffer clear color before gamma correction: Color4(0.046, 0.046, 0.046, 1.0)
    m_command_buffer->beginPass(framebuffer, Color4(0.251, 0.251, 0.251, 1.0)); // after gamma correction

    Mat4 light_ref_matrix = context.frameData().directional_lights.front().lightProjMatrix *
                            context.frameData().directional_lights.front().lightViewMatrix;
    Vec3 light_direction = context.frameData().directional_lights.front().direction;
    Vec3 light_color = context.frameData().directional_lights.front().color;

    const ShaderType shader_type = m_pbr ? ShaderType::PBRShader : ShaderType::BlinnPhongShader;
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(shader_type));
    ShaderResourceBindings bindings;
    int k = 0;
    for (const auto &render_point_light_data : context.frameData().point_lights)
    {
        if (k >= static_cast<int>(MAX_CUBE_SHADOW_MAP_COUNT))
            break;
        std::string light_id = std::string("pointLights[") + std::to_string(k) + "]";
        bindings.setFloat3(light_id + ".position", render_point_light_data.position);
        bindings.setFloat3(light_id + ".color", render_point_light_data.color);
        bindings.setFloat(light_id + ".radius", render_point_light_data.radius);
        bindings.setInt(light_id + ".shadowIndex", render_point_light_data.shadow_index);
        k++;
    }
    bindings.setInt("point_lights_size", k);

    // IBL 环境光（forward 纹理单元：cube shadow 6-10 之后，IBL 固定 11/12/13）。
    const IBLResources& ibl = context.builtinResources().ibl;
    const bool ibl_ready = m_pbr && m_ibl && ibl.isReady();
    bindings.setBool("iblEnable", ibl_ready);
    RhiTexture* default_cube = RenderTextureResource::defaultCubeTexture()->texture();
    RhiTexture* default_2d = RenderTextureResource::defaultTexture()->texture();
    bindings.setCubeTexture("irradianceMap", 11, ibl_ready ? ibl.irradiance_cube : default_cube);
    bindings.setCubeTexture("prefilterMap", 12, ibl_ready ? ibl.prefilter_cube : default_cube);
    bindings.setTexture("brdfLUT", 13, ibl_ready ? ibl.brdf_lut : default_2d);

    for (const RenderMeshSection* render_node : context.renderScene().visibleMeshSections())
    {
        if (!render_node)
            continue;

        const auto &material = render_node->material;
        if (m_pbr)
        {
            bindings.setFloat3("base_color_factor", material.base_color_factor);
            bindings.setFloat("metallic_factor", material.metallic_factor);
            bindings.setFloat("roughness_factor", material.roughness_factor);
            bindings.setFloat("ao_factor", material.ao_factor);
            bindings.setTexture("albedo_map", 0, material.albedoMap());
            bindings.setTexture("metallic_map", 1, material.metallicMap());
            bindings.setTexture("roughness_map", 2, material.roughnessMap());
            bindings.setTexture("ao_map", 3, material.aoMap());
            bindings.setTexture("normal_map", 4, material.normalMap());
            bindings.setBool("has_normal_map", material.has_normal_map);
        }
        else
        {
            bindings.setFloat3("diffuse_factor", material.diffuse_factor);
            bindings.setFloat3("specular_factor", material.specular_factor);
            bindings.setFloat("shininess", material.shininess);
            bindings.setTexture("material.diffuse_map", 0, material.diffuseMap());
            bindings.setTexture("material.specular_map", 1, material.specularMap());
            bindings.setTexture("material.normal_map", 2, material.normalMap());
            bindings.setTexture("material.height_map", 3, material.heightMap());
        }

        bindings.setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty())
        {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            bindings.setInt("bone_count", bone_count);
            bindings.setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else
        {
            bindings.setInt("bone_count", 0);
        }

        bindings.setMatrix("model", 1, render_node->model_matrix);
        bindings.setMatrix("view", 1, context.frameData().view_matrix);
        bindings.setMatrix("projection", 1, context.frameData().proj_matrix);
        bindings.setFloat3("cameraPos", context.frameData().camera_position);

        bindings.setFloat3("directionalLight.direction", light_direction);
        bindings.setFloat3("directionalLight.color", light_color);

        RhiTexture* shadow_texture = hasSlot("inShadowDepth") ? context.texture(slot("inShadowDepth")) : nullptr;
        const bool directional_shadow_ready = m_directional_shadow && shadow_texture != nullptr;
        bindings.setBool("directionalShadowEnable", directional_shadow_ready);
        if (directional_shadow_ready)
        {
            bindings.setMatrix("lightSpaceMatrix", 1, light_ref_matrix);
            bindings.setTexture("shadow_map", 5, shadow_texture);
        }
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}
