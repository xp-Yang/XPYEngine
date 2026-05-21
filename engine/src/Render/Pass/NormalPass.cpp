#include "NormalPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

NormalPass::NormalPass()
{
    m_type = RenderPass::Type::Normal;
    init();
}

void NormalPass::init()
{
}

void NormalPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* target_framebuffer = context.frameBuffer(RGResource::SceneColor);
    if (!target_framebuffer)
        return;
    m_command_buffer->beginPass(target_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    static RenderShaderObject* normal_shader = RenderShaderObject::getShaderObject(ShaderType::NormalShader);
    m_command_buffer->setGraphicsPipeline(normal_shader->graphicsPipeline());
    normal_shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
    normal_shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    normal_shader->setMatrix("projectionView", 1, context.renderSourceData().proj_matrix * context.renderSourceData().view_matrix);
    for (const auto& pair : context.renderSourceData().render_mesh_nodes) {
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
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}
