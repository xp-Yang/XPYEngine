#include "ShadowPass.hpp"
#include <algorithm>
#include <glad/glad.h>

ShadowPass::ShadowPass()
{
    m_type = RenderPass::Type::Shadow;
    init();
}

void ShadowPass::init()
{
	rebuildFramebuffer(Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
}

void ShadowPass::rebuildFramebuffer(const Vec2 &pixel_size, size_t cube_map_count)
{
	Vec2 sz = clampFramebufferPixelSize(pixel_size);
	m_point_shadow_cube_edge = std::clamp(std::min((int)sz.x, (int)sz.y), 256, 4096);

	if (m_framebuffer)
	{
		m_framebuffer->destroyGPU();
		m_framebuffer.reset();
	}

	RhiTexture *color_texture = m_rhi->newTexture(RhiTexture::Format::RGB16F, sz);
	RhiTexture *depth_texture = m_rhi->newTexture(RhiTexture::Format::DEPTH, sz);
	color_texture->create();
	depth_texture->create();
	RhiAttachment color_attachment = RhiAttachment(color_texture);
	RhiAttachment depth_ttachment = RhiAttachment(depth_texture);
	RhiFrameBuffer *fb = m_rhi->newFrameBuffer(color_attachment, sz);
	fb->setDepthAttachment(depth_ttachment);
	fb->create();
	m_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);

	for (unsigned id : m_cube_maps)
		if (id != 0)
			glDeleteTextures(1, &id);
	m_cube_maps.clear();

	if (m_cube_map_fbo == 0)
	{
		m_cube_map_fbo = m_rhi->newFramebufferHandle();
		m_rhi->bindFramebuffer(m_cube_map_fbo);
		m_rhi->setFramebufferDrawReadNone();
	}

	reinit_cube_maps(std::max(cube_map_count, size_t(8)));
}

void ShadowPass::rebuildFramebuffers(const Vec2 &pixel_size)
{
	Vec2 sz = clampFramebufferPixelSize(pixel_size);
	if (m_framebuffer && (int)m_framebuffer->pixelSize().x == (int)sz.x && (int)m_framebuffer->pixelSize().y == (int)sz.y)
		return;

	const size_t cube_reserve = std::max(m_cube_maps.size(), size_t(8));
	rebuildFramebuffer(sz, cube_reserve);
}

void ShadowPass::draw()
{
    drawDirectionalLightShadowMap();
    drawPointLightShadowMap();
}

void ShadowPass::clear()
{
    m_framebuffer->bind();
    m_framebuffer->clear();

    m_rhi->bindFramebuffer(m_cube_map_fbo);
    for (const auto &cube_map : m_cube_maps)
        for (int i = 0; i < 6; i++)
        {
            m_rhi->attachDepthCubeFace(cube_map, i);
            m_rhi->clearColorDepthStencil(1.0f, 1.0f, 1.0f, 1.0f);
        }
    m_rhi->bindDefaultFramebuffer();
}

void ShadowPass::drawDirectionalLightShadowMap()
{
    m_framebuffer->bind();
    m_framebuffer->clear();

    static RenderShaderObject *depth_shader = RenderShaderObject::getShaderObject(ShaderType::OneColorShader);
    depth_shader->start_using();
    Mat4 light_view = m_render_source_data->render_directional_light_data_list.front().lightViewMatrix;
    Mat4 light_proj = m_render_source_data->render_directional_light_data_list.front().lightProjMatrix;
    for (const auto &pair : m_render_source_data->render_mesh_nodes)
    {
        const auto &render_node = pair.second;
        depth_shader->setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty())
        {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            depth_shader->setInt("bone_count", bone_count);
            depth_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else
        {
            depth_shader->setInt("bone_count", 0);
        }
        depth_shader->setMatrix("model", 1, render_node->model_matrix);
        depth_shader->setMatrix("view", 1, light_view);
        depth_shader->setMatrix("projection", 1, light_proj);
        depth_shader->setFloat4("color", Color4(1.0));
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
}

void ShadowPass::drawPointLightShadowMap()
{
    m_rhi->bindFramebuffer(m_cube_map_fbo);
    m_rhi->setViewport(0, 0, m_point_shadow_cube_edge, m_point_shadow_cube_edge);

    static RenderShaderObject *depth_shader = RenderShaderObject::getShaderObject(ShaderType::CubeMapShader);

    depth_shader->start_using();

    std::vector<std::array<Mat4, 6>> light_view;
    std::vector<Mat4> light_proj;
    std::vector<Vec3> light_pos;
    std::vector<float> light_radius;
    for (const auto &render_point_light_data : m_render_source_data->render_point_light_data_list)
    {
        light_view.push_back(render_point_light_data.lightViewMatrix);
        light_proj.push_back(render_point_light_data.lightProjMatrix);
        light_pos.push_back(render_point_light_data.position);
        light_radius.push_back(render_point_light_data.radius);
    }

    if (m_render_source_data->render_point_light_data_list.size() > m_cube_maps.size())
    {
        reinit_cube_maps(m_render_source_data->render_point_light_data_list.size());
    }

    for (int cube_map_id = 0; cube_map_id < m_render_source_data->render_point_light_data_list.size(); cube_map_id++)
    {
        for (int i = 0; i < 6; i++)
        {
            m_rhi->attachDepthCubeFace(m_cube_maps[cube_map_id], i);
            m_rhi->clearColorDepthStencil(1.0f, 1.0f, 1.0f, 1.0f);

            for (const auto &pair : m_render_source_data->render_mesh_nodes)
            {
                const auto &render_node = pair.second;
                depth_shader->setBool("useSkinning", render_node->use_skinning);
                if (render_node->use_skinning && !render_node->bone_matrices.empty())
                {
                    int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
                    depth_shader->setInt("bone_count", bone_count);
                    depth_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
                }
                else
                {
                    depth_shader->setInt("bone_count", 0);
                }
                depth_shader->setMatrix("model", 1, render_node->model_matrix);
                depth_shader->setMatrix("view", 1, light_view[cube_map_id][i]);
                depth_shader->setMatrix("projection", 1, light_proj[cube_map_id]);
                depth_shader->setFloat3("lightPos", light_pos[cube_map_id]);
                depth_shader->setFloat("far_plane", light_radius[cube_map_id]);
                m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
            }
        }
    }
}

void ShadowPass::reinit_cube_maps(size_t count)
{
	m_rhi->bindFramebuffer(m_cube_map_fbo);

	for (unsigned id : m_cube_maps)
		if (id != 0)
			glDeleteTextures(1, &id);

	m_cube_maps.assign(count, 0);
	for (size_t i = 0; i < m_cube_maps.size(); i++)
		m_cube_maps[i] = m_rhi->newDepthCubeMap(m_point_shadow_cube_edge);
}
