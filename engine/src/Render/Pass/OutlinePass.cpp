#include "OutlinePass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

OutlinePass::OutlinePass()
{
    m_type = RenderPass::Type::Outline;
}

void OutlinePass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* mask_framebuffer = context.targetFrameBuffer(RGTarget::OutlineMask);
    if (!mask_framebuffer)
        return;
    mask_framebuffer->bind();
    mask_framebuffer->clear();

    if (context.renderSourceData().picked_ids.empty()) {
        return;
    }
    for (auto picked_id : context.renderSourceData().picked_ids) {
        // render the picked one
        static RenderShaderObject* one_color_shader = RenderShaderObject::getShaderObject(ShaderType::OneColorShader);
        one_color_shader->start_using();
        one_color_shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
        one_color_shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);

        auto it = std::find_if(context.renderSourceData().render_mesh_nodes.begin(), context.renderSourceData().render_mesh_nodes.end(),
            [picked_id](const std::pair<const RenderMeshNodeID, std::shared_ptr<RenderMeshNode>>& pair) {
                return pair.second->node_id.object_id == picked_id;
            }
        );
        if (it != context.renderSourceData().render_mesh_nodes.end()) {
            const auto& render_node = it->second;
            one_color_shader->setBool("useSkinning", render_node->use_skinning);
            if (render_node->use_skinning && !render_node->bone_matrices.empty()) {
                int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
                one_color_shader->setInt("bone_count", bone_count);
                one_color_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
            }
            else {
                one_color_shader->setInt("bone_count", 0);
            }
            one_color_shader->setMatrix("model", 1, render_node->model_matrix);
            int id = picked_id;
            int r = (id & 0x000000FF) >> 0;
            int g = (id & 0x0000FF00) >> 8;
            int b = (id & 0x00FF0000) >> 16;
            Color4 color(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
            one_color_shader->setFloat4("color", color);
            m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
        }
    }
    auto source_map = mask_framebuffer->colorAttachmentAt(0)->texture()->id();


    RhiFrameBuffer* target_framebuffer = context.frameBuffer(RGResource::SceneColor);
    if (!target_framebuffer)
        return;
    target_framebuffer->bind();
    auto obj_depth_map = mask_framebuffer->depthAttachment()->texture()->id();
    static RenderShaderObject* outline_shader = RenderShaderObject::getShaderObject(ShaderType::OutlineShader);
    outline_shader->start_using();
    outline_shader->setTexture("objMap", 0, source_map);
    outline_shader->setTexture("objDepthMap", 1, obj_depth_map);
    m_rhi->drawIndexed(context.renderSourceData().screen_quad->getVAO(), context.renderSourceData().screen_quad->indicesCount());
}
