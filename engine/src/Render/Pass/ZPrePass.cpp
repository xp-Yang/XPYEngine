#include "ZPrePass.hpp"

ZPrePass::ZPrePass()
{
    m_type = RenderPass::Type::ZPre;
    init();
}

void ZPrePass::init()
{
	rebuildFramebuffer(Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
}

void ZPrePass::rebuildFramebuffer(const Vec2 &pixel_size)
{
	RhiTexture *depth_texture = m_rhi->newTexture(RhiTexture::Format::DEPTH, pixel_size);
	depth_texture->create();
	RhiAttachment depth_ttachment = RhiAttachment(depth_texture);
	RhiFrameBuffer *fb = m_rhi->newFrameBuffer(RhiAttachment(), pixel_size);
	fb->setDepthAttachment(depth_ttachment);
	fb->create();
	m_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);
}

void ZPrePass::rebuildFramebuffers(const Vec2 &pixel_size)
{
	Vec2 sz = clampFramebufferPixelSize(pixel_size);
	if (m_framebuffer && (int)m_framebuffer->pixelSize().x == (int)sz.x && (int)m_framebuffer->pixelSize().y == (int)sz.y)
		return;
	if (m_framebuffer)
	{
		m_framebuffer->destroyGPU();
		m_framebuffer.reset();
	}
	rebuildFramebuffer(sz);
}

void ZPrePass::draw()
{
    m_framebuffer->bind();
    m_framebuffer->clear();

    static RenderShaderObject* depth_shader = RenderShaderObject::getShaderObject(ShaderType::OneColorShader);
    depth_shader->start_using();
    Mat4 light_view = m_render_source_data->render_directional_light_data_list.front().lightViewMatrix;
    Mat4 light_proj = m_render_source_data->render_directional_light_data_list.front().lightProjMatrix;
    for (const auto& pair : m_render_source_data->render_mesh_nodes) {
        const auto& render_node = pair.second;
        if (render_node->material.alpha != 1.0f)
            continue;
        depth_shader->setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty()) {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            depth_shader->setInt("bone_count", bone_count);
            depth_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else {
            depth_shader->setInt("bone_count", 0);
        }
        depth_shader->setMatrix("model", 1, render_node->model_matrix);
        depth_shader->setMatrix("view", 1, light_view);
        depth_shader->setMatrix("projection", 1, light_proj);
        depth_shader->setFloat4("color", Color4(1.0));
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
}
