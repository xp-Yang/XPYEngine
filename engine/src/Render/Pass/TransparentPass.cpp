#include "TransparentPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

TransparentPass::TransparentPass()
{
    m_type = RenderPass::Type::Transparent;
    init();
}

void TransparentPass::init()
{
}

void TransparentPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* target_framebuffer = context.frameBuffer(RGResource::SceneColor);
    m_command_buffer->beginPass(target_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    Mat4 light_ref_matrix = context.renderSourceData().render_directional_light_data_list.front().lightProjMatrix *
        context.renderSourceData().render_directional_light_data_list.front().lightViewMatrix;
    Vec3 light_direction = context.renderSourceData().render_directional_light_data_list.front().direction;
    Vec3 light_color = context.renderSourceData().render_directional_light_data_list.front().color;

    RenderPipelineState state;
    state.blend = true;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::TransparentShader, state));
    ShaderResourceBindings bindings;
    bindings.setMatrix("view", 1, context.renderSourceData().view_matrix);
    bindings.setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    for (const auto& pair : context.renderSourceData().render_mesh_nodes) {
        const auto& render_node = pair.second;
        if (render_node->material.alpha == 1.0f)
            continue;

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

        auto& material = render_node->material;

        bindings.setFloat3("diffuse_factor", material.diffuse_factor);
        bindings.setFloat3("specular_factor", material.specular_factor);
        bindings.setFloat("shininess", material.shininess);
        bindings.setTexture("material.diffuse_map", 0, material.diffuse_map);
        bindings.setTexture("material.specular_map", 1, material.specular_map);
        bindings.setTexture("material.normal_map", 2, material.normal_map);

        bindings.setFloat3("cameraPos", context.renderSourceData().camera_position);

        bindings.setFloat3("directionalLight.direction", light_direction);
        bindings.setFloat3("directionalLight.color", light_color);

        bindings.setFloat("alpha", material.alpha);
        m_command_buffer->setShaderResources(&bindings);

        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}
