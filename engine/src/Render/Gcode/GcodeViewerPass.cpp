#include "GcodeViewerPass.hpp"

#include <glad/glad.h>

GcodeViewerPass::GcodeViewerPass() {
	init();
}

void GcodeViewerPass::init()
{
	RhiTexture* color_texture = m_rhi->newTexture(RhiTexture::Format::RGB8, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y), 4);
	RhiTexture* depth_texture = m_rhi->newTexture(RhiTexture::Format::DEPTH24STENCIL8, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y), 4);
	color_texture->create();
	depth_texture->create();
	RhiAttachment color_attachment = RhiAttachment(color_texture);
	RhiAttachment depth_ttachment = RhiAttachment(depth_texture);
	RhiFrameBuffer* fb = m_rhi->newFrameBuffer(color_attachment, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y), 4);
	fb->setDepthAttachment(depth_ttachment);
	fb->create();
	m_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);
}

void GcodeViewerPass::reload_mesh_data(std::array<LinesBatch, ExtrusionRole::erCount> lines_batches)
{
	for (int i = 0; i < m_VAOs.size(); i++) {
		// delete all the gl buffer
		glDeleteVertexArrays(1, &m_VAOs[i]);
		glDeleteBuffers(1, &m_VBOs[i]);
		glDeleteBuffers(1, &m_IBOs[i]);
	}
	m_VAOs.clear();
	m_VBOs.clear();
	m_IBOs.clear();
	m_colors.clear();

	for (int i = 0; i < lines_batches.size(); i++) {
		if (lines_batches[i].empty()) {
			m_VAOs.push_back(0);
			m_VBOs.push_back(0);
			m_IBOs.push_back(0);
			m_colors.push_back({});
			continue;
		}

		const auto& mesh = lines_batches[i].merged_mesh;

		unsigned int VBO = 0;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, mesh->vertices.size() * sizeof(SimpleVertex), &(mesh->vertices[0]), GL_STATIC_DRAW);
		m_VBOs.push_back(VBO);

		unsigned int IBO = 0;
		glGenBuffers(1, &IBO);
		glBindBuffer(GL_ARRAY_BUFFER, IBO);
		glBufferData(GL_ARRAY_BUFFER, mesh->indices.size() * sizeof(int), &(mesh->indices[0]), GL_STATIC_DRAW);
		m_IBOs.push_back(IBO);

		unsigned int VAO = 0;
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)offsetof(SimpleVertex, normal));
		glEnableVertexAttribArray(1);
		m_VAOs.push_back(VAO);

		m_colors.push_back(Extrusion_Role_Colors[i]);
	}
}

void GcodeViewerPass::draw()
{
	m_framebuffer->bind();
	m_framebuffer->clear(Color4(0.251f, 0.251f, 0.251f, 1.0f));

	static RenderShaderObject* shader = RenderShaderObject::getShaderObject(ShaderType::GcodeShader);
	shader->start_using();
	shader->setMatrix("model", 1, Math::Rotate(-0.5 * Math::Constant::PI, Vec3(1, 0, 0)) * Math::Scale(Vec3(50.0f / 256.0f)) * Math::Translate(Vec3(-128.0f, -128.0f, 0.0f)));
	shader->setMatrix("view", 1, m_render_source_data->view_matrix);
	shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
	shader->setFloat("material.ambient", 0.2f);
	shader->setFloat3("material.specular", Vec3(0.2f));
	float layer_height = (m_gcode_viewer && m_gcode_viewer->get_layers().size() > 0) ?
		m_gcode_viewer->get_layers().back().height / m_gcode_viewer->get_layers().size() : 1.0f;
	layer_height *= 50.0f / 256.0f;
	shader->setFloat("layerHeight", layer_height);
	for (int i = 0; i < m_VAOs.size(); i++) {
		if (m_VAOs[i] == 0)
			continue;

		if (i == ExtrusionRole::erInternalInfill)
			continue;

		const auto& index_offset = m_gcode_viewer->colored_indices_interval(ExtrusionRole(i));
		int start_offset = index_offset.first;
		int size = index_offset.second - index_offset.first;
		shader->setFloat3("material.diffuse", Vec3(m_colors[i]));
		glBindVertexArray(m_VAOs[i]);
		glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, (const void*)(start_offset * sizeof(int)));

		{
			const auto& index_offset = m_gcode_viewer->colorless_indices_interval(ExtrusionRole(i));
			int start_offset = index_offset.first;
			int size = index_offset.second - index_offset.first;
			if (size > 0) {
				shader->setFloat3("material.diffuse", Vec3(Silent_Color));
				glBindVertexArray(m_VAOs[i]);
				glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, (const void*)(start_offset * sizeof(int)));
			}
		}
	}
	// render ExtrusionRole::erInternalInfill in the final
	if (m_VAOs.size() == ExtrusionRole::erCount &&
		m_VAOs[ExtrusionRole::erInternalInfill] != 0)
	{
		const auto& index_offset = m_gcode_viewer->colored_indices_interval(ExtrusionRole::erInternalInfill);
		int start_offset = index_offset.first;
		int size = index_offset.second - index_offset.first;
		shader->setFloat3("material.diffuse", Vec3(m_colors[ExtrusionRole::erInternalInfill]));
		glBindVertexArray(m_VAOs[ExtrusionRole::erInternalInfill]);
		glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, (const void*)(start_offset * sizeof(int)));

		{
			const auto& index_offset = m_gcode_viewer->colorless_indices_interval(ExtrusionRole::erInternalInfill);
			int start_offset = index_offset.first;
			int size = index_offset.second - index_offset.first;
			if (size > 0) {
				shader->setFloat3("material.diffuse", Vec3(Silent_Color));
				glBindVertexArray(m_VAOs[ExtrusionRole::erInternalInfill]);
				glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, (const void*)(start_offset * sizeof(int)));
			}
		}
	}
	shader->stop_using();

	{
		// draw the ground
		static RenderShaderObject* shader = RenderShaderObject::getShaderObject(ShaderType::PBRShader);
		shader->start_using();
		for (const auto& pair : m_render_source_data->render_mesh_nodes) {
			if (pair.first.object_id != 1)
				continue;

			const auto& render_node = pair.second;
			auto& material = render_node->material;
			shader->setFloat3("albedo", Vec3(0.25f));
			shader->setFloat("metallic", 0.0f);
			shader->setFloat("roughness", 1.0f);
			shader->setFloat("ao", 0.0f);

			shader->setMatrix("model", 1, render_node->model_matrix);
			shader->setMatrix("view", 1, m_render_source_data->view_matrix);
			shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
			shader->setFloat3("cameraPos", m_render_source_data->camera_position);

			Vec3 light_direction = m_render_source_data->render_directional_light_data_list.front().direction;
			Vec4 light_color = m_render_source_data->render_directional_light_data_list.front().color;
			shader->setFloat3("directionalLight.direction", light_direction);
			shader->setFloat4("directionalLight.color", light_color);

			m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->mesh.indicesCount());
		}
		shader->stop_using();
	}
}
