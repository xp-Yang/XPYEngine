#include "ZPrePass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

ZPrePass::ZPrePass()
{
    m_type = RenderPass::Type::ZPre;
}

void ZPrePass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.frameBufferOfTarget(RGTarget::Main);
    if (!framebuffer)
        return;
    m_command_buffer->beginPass(framebuffer);

    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::SingleColorShader));
    ShaderResourceBindings bindings;
    Mat4 light_view = context.frameData().directional_lights.front().lightViewMatrix;
    Mat4 light_proj = context.frameData().directional_lights.front().lightProjMatrix;
    for (const RenderMeshSection* render_node : context.renderScene().opaqueMeshSections()) {
        if (!render_node)
            continue;
        bindings.setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty()) {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            bindings.setInt("bone_count", bone_count);
            bindings.setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else {
            bindings.setInt("bone_count", 0);
        }
        bindings.setMatrix("model", 1, render_node->model_matrix);
        bindings.setMatrix("view", 1, light_view);
        bindings.setMatrix("projection", 1, light_proj);
        bindings.setFloat4("color", Color4(1.0));
        m_command_buffer->setShaderResources(&bindings);
        m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
        m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
    }
    m_command_buffer->endPass();
}
