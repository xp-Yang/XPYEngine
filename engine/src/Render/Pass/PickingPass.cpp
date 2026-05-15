#include "PickingPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

PickingPass::PickingPass()
{
    m_type = RenderPass::Type::Picking;
}

void PickingPass::draw(RenderPassContext& context)
{
    // render for picking
    RhiFrameBuffer* framebuffer = context.targetFrameBuffer();
    if (!framebuffer)
        return;
    framebuffer->bind();
    framebuffer->clear();

    static RenderShaderObject* picking_shader = RenderShaderObject::getShaderObject(ShaderType::OneColorShader);
    picking_shader->start_using();

    picking_shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
    picking_shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);

    // TODO Unpickable
    for (const auto& pair : context.renderSourceData().render_mesh_nodes) {
        const auto& render_node = pair.second;
        picking_shader->setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty()) {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            picking_shader->setInt("bone_count", bone_count);
            picking_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else {
            picking_shader->setInt("bone_count", 0);
        }
        picking_shader->setMatrix("model", 1, render_node->model_matrix);
        int id = render_node->node_id.object_id;
        int r = (id & 0x000000FF) >> 0;
        int g = (id & 0x0000FF00) >> 8;
        int b = (id & 0x00FF0000) >> 16;
        Color4 color(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
        picking_shader->setFloat4("color", color);
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }

    framebuffer->unBind();
}
