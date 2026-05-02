#include "NormalPass.hpp"

NormalPass::NormalPass()
{
    m_type = RenderPass::Type::Normal;
    init();
}

void NormalPass::init()
{
}

void NormalPass::draw()
{
    m_input_passes[0]->getFrameBuffer()->bind();

    static RenderShaderObject* normal_shader = RenderShaderObject::getShaderObject(ShaderType::NormalShader);
    normal_shader->start_using();
    normal_shader->setMatrix("view", 1, m_render_source_data->view_matrix);
    normal_shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
    normal_shader->setMatrix("projectionView", 1, m_render_source_data->proj_matrix * m_render_source_data->view_matrix);
    for (const auto& pair : m_render_source_data->render_mesh_nodes) {
        const auto& render_node = pair.second;
        normal_shader->setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty())
        {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            normal_shader->setInt("bone_count", bone_count);
            normal_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else
        {
            normal_shader->setInt("bone_count", 0);
        }
        normal_shader->setMatrix("model", 1, render_node->model_matrix);
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
}
