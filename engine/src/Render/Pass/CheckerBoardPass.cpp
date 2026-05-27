#include "CheckerBoardPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

CheckerBoardPass::CheckerBoardPass()
{
    m_type = RenderPass::Type::CheckerBoard;
}

void CheckerBoardPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);
    if (!framebuffer)
        return;
    m_command_buffer->beginPass(framebuffer);

    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::CheckerboardShader));
    ShaderResourceBindings bindings;
    bindings.setMatrix("view", 1, context.renderSourceData().view_matrix);
    bindings.setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    for (const auto& pair : context.renderSourceData().render_mesh_nodes) {
        const auto& render_node = pair.second;
        Mat4 modelScale;
        Mat4 modelRotation;
        Mat4 modelTranslation;
        Math::DecomposeMatrix(render_node->model_matrix, modelTranslation, modelRotation, modelScale);
        bindings.setMatrix("modelScale", 1, modelScale);
        bindings.setMatrix("model", 1, render_node->model_matrix);
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}
