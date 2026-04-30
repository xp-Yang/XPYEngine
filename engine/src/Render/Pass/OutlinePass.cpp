#include "OutlinePass.hpp"

OutlinePass::OutlinePass()
{
    m_type = RenderPass::Type::Outline;
    init();
}

void OutlinePass::init()
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
        m_source_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);
    }
}

void OutlinePass::draw()
{
    m_source_framebuffer->bind();
    m_source_framebuffer->clear();

    if (m_render_source_data->picked_ids.empty()) {
        m_framebuffer->bind();
        m_framebuffer->clear();
        return;
    }
    for (auto picked_id : m_render_source_data->picked_ids) {
        // render the picked one
        static RenderShaderObject* one_color_shader = RenderShaderObject::getShaderObject(ShaderType::OneColorShader);
        one_color_shader->start_using();
        one_color_shader->setMatrix("view", 1, m_render_source_data->view_matrix);
        one_color_shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);

        auto it = std::find_if(m_render_source_data->render_mesh_nodes.begin(), m_render_source_data->render_mesh_nodes.end(),
            [picked_id](const std::pair<const RenderMeshNodeID, std::shared_ptr<RenderMeshNode>>& pair) {
                return pair.second->node_id.object_id == picked_id;
            }
        );
        if (it != m_render_source_data->render_mesh_nodes.end()) {
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
            m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->mesh.indicesCount());
        }
    }
    auto source_map = m_source_framebuffer->colorAttachmentAt(0)->texture()->id();


    m_input_passes[0]->getFrameBuffer()->bind();
    auto obj_depth_map = m_source_framebuffer->depthAttachment()->texture()->id();
    static RenderShaderObject* outline_shader = RenderShaderObject::getShaderObject(ShaderType::OutlineShader);
    outline_shader->start_using();
    outline_shader->setTexture("objMap", 0, source_map);
    outline_shader->setTexture("objDepthMap", 1, obj_depth_map);
    m_rhi->drawIndexed(m_render_source_data->screen_quad->getVAO(), m_render_source_data->screen_quad->indicesCount());
}
