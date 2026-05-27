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
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);
    if (!framebuffer)
        return;
    // framebuffer clear color before gamma correction: Color4(0.046, 0.046, 0.046, 1.0)
    m_command_buffer->beginPass(framebuffer, Color4(0.251, 0.251, 0.251, 1.0)); // after gamma correction

    Mat4 light_ref_matrix = context.renderSourceData().render_directional_light_data_list.front().lightProjMatrix *
                            context.renderSourceData().render_directional_light_data_list.front().lightViewMatrix;
    Vec3 light_direction = context.renderSourceData().render_directional_light_data_list.front().direction;
    Vec3 light_color = context.renderSourceData().render_directional_light_data_list.front().color;

    const ShaderType shader_type = m_pbr ? ShaderType::PBRShader : ShaderType::BlinnPhongShader;
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(shader_type));
    ShaderResourceBindings bindings;
    int k = 0;
    for (const auto &render_point_light_data : context.renderSourceData().render_point_light_data_list)
    {
        if (k > MAX_CUBE_SHADOW_MAP_COUNT)
            break;
        std::string light_id = std::string("pointLights[") + std::to_string(k) + "]";
        bindings.setFloat3(light_id + ".position", render_point_light_data.position);
        bindings.setFloat3(light_id + ".color", render_point_light_data.color);
        bindings.setFloat(light_id + ".radius", render_point_light_data.radius);
        k++;
    }
    bindings.setInt("point_lights_size", k);
    for (const auto &pair : context.renderSourceData().render_mesh_nodes)
    {
        const auto &render_node = pair.second;
        auto &material = render_node->material;
        if (m_pbr)
        {
            bindings.setFloat3("base_color_factor", material.base_color_factor);
            bindings.setFloat("metallic_factor", material.metallic_factor);
            bindings.setFloat("roughness_factor", material.roughness_factor);
            bindings.setFloat("ao_factor", material.ao_factor);
            bindings.setTexture("albedo_map", 0, material.albedo_map);
            bindings.setTexture("metallic_map", 1, material.metallic_map);
            bindings.setTexture("roughness_map", 2, material.roughness_map);
            bindings.setTexture("ao_map", 3, material.ao_map);
        }
        else
        {
            bindings.setFloat3("diffuse_factor", material.diffuse_factor);
            bindings.setFloat3("specular_factor", material.specular_factor);
            bindings.setFloat("shininess", material.shininess);
            bindings.setTexture("material.diffuse_map", 0, material.diffuse_map);
            bindings.setTexture("material.specular_map", 1, material.specular_map);
            bindings.setTexture("material.normal_map", 2, material.normal_map);
            bindings.setTexture("material.height_map", 3, material.height_map);
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
        bindings.setMatrix("view", 1, context.renderSourceData().view_matrix);
        bindings.setMatrix("projection", 1, context.renderSourceData().proj_matrix);
        bindings.setFloat3("cameraPos", context.renderSourceData().camera_position);

        bindings.setFloat3("directionalLight.direction", light_direction);
        bindings.setFloat3("directionalLight.color", light_color);

        RhiTexture* shadow_texture = context.texture(RGResource::ShadowDirectionalDepth);
        if (shadow_texture)
        {
            bindings.setMatrix("lightSpaceMatrix", 1, light_ref_matrix);
            bindings.setTexture("shadow_map", 5, shadow_texture);

            std::vector<RhiTexture*> cube_shadow_maps = context.cubeShadowMaps();
            for (int i = 0; i < cube_shadow_maps.size(); i++)
            {
                if (!cube_shadow_maps[i])
                    continue;
                std::string cube_map_id = std::string("cube_shadow_maps[") + std::to_string(i) + "]";
                bindings.setCubeTexture(cube_map_id, 6 + i, cube_shadow_maps[i]);
            }
        }
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}
