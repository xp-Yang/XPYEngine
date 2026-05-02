#include "WireFramePass.hpp"

WireFramePass::WireFramePass()
{
    m_type = RenderPass::Type::WireFrame;
    init();
}

void WireFramePass::init()
{
}

void WireFramePass::draw()
{
    m_input_passes[0]->getFrameBuffer()->bind();

    static RenderShaderObject* wireframe_shader = RenderShaderObject::getShaderObject(ShaderType::WireframeShader);
    wireframe_shader->start_using();
    wireframe_shader->setMatrix("view", 1, m_render_source_data->view_matrix);
    wireframe_shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
    for (const auto& pair : m_render_source_data->render_mesh_nodes) {
        const auto& render_node = pair.second;
        wireframe_shader->setMatrix("model", 1, render_node->model_matrix);
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
}
