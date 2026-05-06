#include "CheckerBoardPass.hpp"

CheckerBoardPass::CheckerBoardPass()
{
    m_type = RenderPass::Type::CheckerBoard;
    init();
}

void CheckerBoardPass::init()
{
	rebuildFramebuffer(Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
}

void CheckerBoardPass::rebuildFramebuffer(const Vec2 &pixel_size)
{
	RhiTexture *color_texture = m_rhi->newTexture(RhiTexture::Format::RGB16F, pixel_size);
	RhiTexture *depth_texture = m_rhi->newTexture(RhiTexture::Format::DEPTH, pixel_size);
	color_texture->create();
	depth_texture->create();
	RhiAttachment color_attachment = RhiAttachment(color_texture);
	RhiAttachment depth_ttachment = RhiAttachment(depth_texture);
	RhiFrameBuffer *fb = m_rhi->newFrameBuffer(color_attachment, pixel_size);
	fb->setDepthAttachment(depth_ttachment);
	fb->create();
	m_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);
}

void CheckerBoardPass::rebuildFramebuffers(const Vec2 &pixel_size)
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

void CheckerBoardPass::draw()
{
    m_framebuffer->bind();
    m_framebuffer->clear();

    static RenderShaderObject* shader = RenderShaderObject::getShaderObject(ShaderType::CheckerboardShader);
    shader->start_using();
    shader->setMatrix("view", 1, m_render_source_data->view_matrix);
    shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
    for (const auto& pair : m_render_source_data->render_mesh_nodes) {
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
