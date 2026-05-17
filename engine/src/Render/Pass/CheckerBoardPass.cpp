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
    framebuffer->bind();
    framebuffer->clear();

    static RenderShaderObject* shader = RenderShaderObject::getShaderObject(ShaderType::CheckerboardShader);
    shader->start_using();
    shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
    shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    for (const auto& pair : context.renderSourceData().render_mesh_nodes) {
        const auto& render_node = pair.second;
        Mat4 modelScale;
        Mat4 modelRotation;
        Mat4 modelTranslation;
        Math::DecomposeMatrix(render_node->model_matrix, modelTranslation, modelRotation, modelScale);
        shader->setMatrix("modelScale", 1, modelScale);
        shader->setMatrix("model", 1, render_node->model_matrix);
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
    shader->stop_using();
}
