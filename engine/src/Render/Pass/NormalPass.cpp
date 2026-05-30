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
    RhiFrameBuffer* target_framebuffer = context.frameBuffer(slot("outColor"));
    if (!target_framebuffer)
        return;
    m_command_buffer->beginPass(target_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::NormalShader));
    ShaderResourceBindings bindings;
    bindings.setMatrix("view", 1, context.frameData().view_matrix);
    bindings.setMatrix("projection", 1, context.frameData().proj_matrix);
    bindings.setMatrix("projectionView", 1, context.frameData().proj_matrix * context.frameData().view_matrix);
    for (const RenderMeshSection* render_node : context.renderScene().visibleMeshSections()) {
        if (!render_node)
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
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}
