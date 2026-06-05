#include "CheckerBoardPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

CheckerBoardPass::CheckerBoardPass()
{
    m_type = RenderPass::Type::CheckerBoard;
}

void CheckerBoardPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(slot("outTarget"));
    if (!framebuffer)
        return;
    m_command_buffer->beginPass(framebuffer);

    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::CheckerboardShader));
    ShaderResourceBindings bindings;
    bindings.setMatrix("view", 1, context.frameData().view_matrix);
    bindings.setMatrix("projection", 1, context.frameData().proj_matrix);
    for (const RenderMeshSection* render_node : context.renderScene().mainCameraVisibleMeshSections()) {
        if (!render_node)
            continue;

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
