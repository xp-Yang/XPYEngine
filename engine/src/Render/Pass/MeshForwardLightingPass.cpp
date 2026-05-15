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
    RhiFrameBuffer* framebuffer = context.targetFrameBuffer();
    if (!framebuffer)
        return;
    framebuffer->bind();
    // framebuffer->clear(Color4(0.046, 0.046, 0.046, 1.0)); // before gamma correction
    framebuffer->clear(Color4(0.251, 0.251, 0.251, 1.0)); // after gamma correction

    Mat4 light_ref_matrix = context.renderSourceData().render_directional_light_data_list.front().lightProjMatrix *
                            context.renderSourceData().render_directional_light_data_list.front().lightViewMatrix;
    Vec3 light_direction = context.renderSourceData().render_directional_light_data_list.front().direction;
    Vec4 light_color = context.renderSourceData().render_directional_light_data_list.front().color;

    RhiTexture* shadow_texture = context.texture(RGResource::ShadowDirectionalDepth);
    m_shadow_map = shadow_texture ? shadow_texture->id() : 0;

    static RenderShaderObject *pbr_shader = RenderShaderObject::getShaderObject(ShaderType::PBRShader);
    static RenderShaderObject *blinn_phong_shader = RenderShaderObject::getShaderObject(ShaderType::BlinnPhongShader);
    RenderShaderObject *shader = m_pbr ? pbr_shader : blinn_phong_shader;
    shader->start_using();
    int k = 0;
    for (const auto &render_point_light_data : context.renderSourceData().render_point_light_data_list)
    {
        std::string light_id = std::string("pointLights[") + std::to_string(k) + "]";
        shader->setFloat3(light_id + ".position", render_point_light_data.position);
        shader->setFloat4(light_id + ".color", render_point_light_data.color);
        shader->setFloat(light_id + ".radius", render_point_light_data.radius);
        k++;
    }
    shader->setInt("point_lights_size", k);
    for (const auto &pair : context.renderSourceData().render_mesh_nodes)
    {
        const auto &render_node = pair.second;
        auto &material = render_node->material;
        if (m_pbr)
        {
            shader->setFloat3("base_color_factor", material.base_color_factor);
            shader->setFloat("metallic_factor", material.metallic_factor);
            shader->setFloat("roughness_factor", material.roughness_factor);
            shader->setFloat("ao_factor", material.ao_factor);
            shader->setTexture("albedo_map", 0, material.albedo_map);
            shader->setTexture("metallic_map", 1, material.metallic_map);
            shader->setTexture("roughness_map", 2, material.roughness_map);
            shader->setTexture("ao_map", 3, material.ao_map);
        }
        else
        {
            shader->setFloat3("diffuse_factor", material.diffuse_factor);
            shader->setFloat3("specular_factor", material.specular_factor);
            shader->setFloat("shininess", material.shininess);
            shader->setTexture("material.diffuse_map", 0, material.diffuse_map);
            shader->setTexture("material.specular_map", 1, material.specular_map);
            shader->setTexture("material.normal_map", 2, material.normal_map);
            shader->setTexture("material.height_map", 3, material.height_map);
        }

        shader->setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty())
        {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            shader->setInt("bone_count", bone_count);
            shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else
        {
            shader->setInt("bone_count", 0);
        }

        shader->setMatrix("model", 1, render_node->model_matrix);
        shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
        shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);
        shader->setFloat3("cameraPos", context.renderSourceData().camera_position);

        shader->setFloat3("directionalLight.direction", light_direction);
        shader->setFloat4("directionalLight.color", light_color);

        if (m_shadow_map != 0)
        {
            shader->setMatrix("lightSpaceMatrix", 1, light_ref_matrix);
            shader->setTexture("shadow_map", 5, m_shadow_map);

            for (int i = 0; i < m_cube_maps.size(); i++)
            {
                std::string cube_map_id = std::string("cube_shadow_maps[") + std::to_string(i) + "]";
                shader->setCubeTexture(cube_map_id, 6 + i, m_cube_maps[i]);
            }
        }
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
    shader->stop_using();
}
