#include "PickingPass.hpp"

PickingPass::PickingPass()
{
    m_type = RenderPass::Type::Picking;
    init();
}

void PickingPass::init()
{
    RhiTexture* color_texture = m_rhi->newTexture(RhiTexture::Format::RGB16F, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    RhiTexture* depth_texture = m_rhi->newTexture(RhiTexture::Format::DEPTH, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    color_texture->create();
    depth_texture->create();
    RhiAttachment color_attachment = RhiAttachment(color_texture);
    RhiAttachment depth_ttachment = RhiAttachment(depth_texture);
    RhiFrameBuffer* fb = m_rhi->newFrameBuffer(color_attachment, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    fb->setDepthAttachment(depth_ttachment);
    fb->create();
    m_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);
}

void PickingPass::draw()
{
    // render for picking
    m_framebuffer->bind();
    m_framebuffer->clear();

    static RenderShaderObject* picking_shader = RenderShaderObject::getShaderObject(ShaderType::OneColorShader);
    picking_shader->start_using();

    picking_shader->setMatrix("view", 1, m_render_source_data->view_matrix);
    picking_shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);

    // TODO Unpickable
    for (const auto& pair : m_render_source_data->render_mesh_nodes) {
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

    m_framebuffer->unBind();
}
