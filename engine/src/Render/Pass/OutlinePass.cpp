#include "OutlinePass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

OutlinePass::OutlinePass()
{
    m_type = RenderPass::Type::Outline;
}

void OutlinePass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* mask_framebuffer = context.frameBufferOfTarget(RGTarget::OutlineMask);
    if (!mask_framebuffer)
        return;
    m_command_buffer->beginPass(mask_framebuffer);

    if (context.renderSourceData().picked_ids.empty()) {
        m_command_buffer->endPass();
        return;
    }
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::SingleColorShader));
    ShaderResourceBindings bindings;
    for (auto picked_id : context.renderSourceData().picked_ids) {
        // render the picked one
        bindings.setMatrix("view", 1, context.renderSourceData().view_matrix);
        bindings.setMatrix("projection", 1, context.renderSourceData().proj_matrix);

        auto it = std::find_if(context.renderSourceData().render_mesh_nodes.begin(), context.renderSourceData().render_mesh_nodes.end(),
            [picked_id](const std::pair<const RenderMeshNodeID, std::shared_ptr<RenderMeshNode>>& pair) {
                return pair.second->node_id.object_id == picked_id;
            }
        );
        if (it != context.renderSourceData().render_mesh_nodes.end()) {
            const auto& render_node = it->second;
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
            int id = picked_id * PickingColorIDFactor;
            int r = (id & 0x000000FF) >> 0;
            int g = (id & 0x0000FF00) >> 8;
            int b = (id & 0x00FF0000) >> 16;
            Color4 color(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
            bindings.setFloat4("color", color);
            m_command_buffer->setShaderResources(&bindings);
            m_command_buffer->setVertexInput(render_node->mesh.vertexLayout());
            m_command_buffer->drawIndexed(render_node->source_index_count, 1, render_node->source_index_offset);
        }
    }
    m_command_buffer->endPass();


    RhiFrameBuffer* target_framebuffer = context.frameBuffer(RGResource::SceneColor);
    if (!target_framebuffer)
        return;
    m_command_buffer->beginPass(target_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);
    auto source_map = mask_framebuffer->colorAttachmentAt(0)->texture()->id();
    auto obj_depth_map = mask_framebuffer->depthAttachment()->texture()->id();
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::OutlineShader));
    ShaderResourceBindings outline_bindings;
    outline_bindings.setTexture("objMap", 0, source_map);
    outline_bindings.setTexture("objDepthMap", 1, obj_depth_map);
    m_command_buffer->setShaderResources(&outline_bindings);
    m_command_buffer->setVertexInput(context.renderSourceData().screen_quad->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(context.renderSourceData().screen_quad->indicesCount()));
    m_command_buffer->endPass();
}
