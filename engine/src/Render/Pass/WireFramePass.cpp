#include "WireFramePass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

WireFramePass::WireFramePass()
{
    m_type = RenderPass::Type::WireFrame;
    init();
}

void WireFramePass::init()
{
}

void WireFramePass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* target_framebuffer = context.frameBuffer(RGResource::SceneColor);
    if (!target_framebuffer)
        return;
    m_command_buffer->beginPass(target_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::WireframeShader));
    ShaderResourceBindings bindings;
    bindings.setMatrix("view", 1, context.renderSourceData().view_matrix);
    bindings.setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    for (const auto& pair : context.renderSourceData().render_mesh_nodes) {
        const auto& render_node = pair.second;
        bindings.setMatrix("model", 1, render_node->model_matrix);
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}
