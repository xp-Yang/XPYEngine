#include "GcodeViewerInstancingPass.hpp"
#include "Logical/Gcode/SimpleMesh.hpp"

#include <glad/glad.h>

namespace Instance {

GcodeViewerInstancingPass::GcodeViewerInstancingPass() {
	init();
}

void GcodeViewerInstancingPass::init()
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

void GcodeViewerInstancingPass::reload_mesh_data(std::array<LinesBatch, ExtrusionRole::erCount> lines_batches)
{
	for (int i = 0; i < m_VAOs.size(); i++) {
		glDeleteVertexArrays(1, &m_VAOs[i]);
	}
	m_VAOs.clear();
	m_colors.clear();

	std::array<Vec3, 2> face_center_pos = { Vec3(0,0,0), Vec3(1,0,0) };
	Vec3 face_left_vec = Vec3(0, 0.5f, 0);
	Vec3 face_up_vec = Vec3(0, 0, 0.5f);
	std::shared_ptr<SimpleMesh> unit_cube_mesh = SimpleMesh::create_vertex_normal_cuboid_mesh(face_center_pos, face_left_vec, face_up_vec);

	unsigned int VBO = 0;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, unit_cube_mesh->vertices.size() * sizeof(SimpleVertex), &(unit_cube_mesh->vertices[0]), GL_STATIC_DRAW);

	unsigned int IBO = 0;
	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ARRAY_BUFFER, IBO);
	glBufferData(GL_ARRAY_BUFFER, unit_cube_mesh->indices.size() * sizeof(int), &(unit_cube_mesh->indices[0]), GL_STATIC_DRAW);

	for (int i = 0; i < lines_batches.size(); i++) {
		if (lines_batches[i].empty()) {
			m_VAOs.push_back(0);
			m_Instances.push_back(0);
			m_colors.push_back({});
			continue;
		}



		unsigned int VAO = 0;
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)offsetof(SimpleVertex, normal));

		unsigned int instance_buffer_id = 0;
		glGenBuffers(1, &instance_buffer_id);
		glBindBuffer(GL_ARRAY_BUFFER, instance_buffer_id);
		glBufferData(GL_ARRAY_BUFFER, lines_batches[i].instance_data_buffer_size * sizeof(inst_data), lines_batches[i].instance_data_buffer, GL_STATIC_DRAW);
		m_Instances.push_back(instance_buffer_id);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 4, GL_FLOAT, false, 4 * sizeof(Vec4), (GLvoid*)(0));
		glVertexAttribDivisor(2, 1);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, false, 4 * sizeof(Vec4), (GLvoid*)(sizeof(Vec4)));
		glVertexAttribDivisor(3, 1);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 4, GL_FLOAT, false, 4 * sizeof(Vec4), (GLvoid*)(2 * sizeof(Vec4)));
		glVertexAttribDivisor(4, 1);
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, false, 4 * sizeof(Vec4), (GLvoid*)(3 * sizeof(Vec4)));
		glVertexAttribDivisor(5, 1);

		m_VAOs.push_back(VAO);

		m_colors.push_back(Extrusion_Role_Colors[i]);
	}
}

void GcodeViewerInstancingPass::draw()
{
	m_framebuffer->bind();
	m_framebuffer->clear(Color4(0.251f, 0.251f, 0.251f, 1.0f));

	static RenderShaderObject* shader = RenderShaderObject::getShaderObject(ShaderType::GcodeInstancingShader);
	shader->start_using();
	shader->setMatrix("view", 1, m_render_source_data->view_matrix *
		Math::Rotate(-0.5 * Math::Constant::PI, Vec3(1, 0, 0)) * Math::Scale(Vec3(50.0f / 256.0f)) * Math::Translate(Vec3(-128.0f, -128.0f, 0.0f)));
	shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
	for (int i = 0; i < m_VAOs.size(); i++) {
		if (m_VAOs[i] == 0)
			continue;

		const auto& index_offset = m_gcode_viewer->colored_indices_interval(ExtrusionRole(i));
		int start_offset = index_offset.first;
		int size = index_offset.second - index_offset.first;

		shader->setFloat("material.ambient", 0.2f);
		shader->setFloat3("material.diffuse", Vec3(m_colors[i]));
		shader->setFloat3("material.specular", Vec3(0.2f));

		size_t intance_count = m_gcode_viewer->linesBatches()[i].instance_data_buffer_size;
		glBindVertexArray(m_VAOs[i]);
		glDrawElementsInstanced(GL_TRIANGLES, size, GL_UNSIGNED_INT, (const void*)(start_offset * sizeof(int)), intance_count);

		{
			const auto& index_offset = m_gcode_viewer->colorless_indices_interval(ExtrusionRole(i));
			int start_offset = index_offset.first;
			int size = index_offset.second - index_offset.first;
			if (size > 0) {
				shader->setFloat3("material.diffuse", Vec3(Silent_Color));

				glBindVertexArray(m_VAOs[i]);
				glDrawElementsInstanced(GL_TRIANGLES, size, GL_UNSIGNED_INT, (const void*)(start_offset * sizeof(int)), intance_count);
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
}
