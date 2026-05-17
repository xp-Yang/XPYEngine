#include "TransparentPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

TransparentPass::TransparentPass()
{
    m_type = RenderPass::Type::Transparent;
    init();
}

void TransparentPass::init()
{
}

void TransparentPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* target_framebuffer = context.frameBuffer(RGResource::SceneColor);
    target_framebuffer->bind();

    m_rhi->setBlend(true);
    m_rhi->setDepthMask(false);

    static auto shader_ = Shader{ std::string(RESOURCE_DIR) + "/shader/mesh.vs", std::string(RESOURCE_DIR) + "/shader/transparent.fs"};
    static RenderShaderObject* shader = new RenderShaderObject(shader_);

    Mat4 light_ref_matrix = context.renderSourceData().render_directional_light_data_list.front().lightProjMatrix *
        context.renderSourceData().render_directional_light_data_list.front().lightViewMatrix;
    Vec3 light_direction = context.renderSourceData().render_directional_light_data_list.front().direction;
    Vec4 light_color = context.renderSourceData().render_directional_light_data_list.front().color;

    shader->start_using();
    shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
    shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    for (const auto& pair : context.renderSourceData().render_mesh_nodes) {
        const auto& render_node = pair.second;
        if (render_node->material.alpha == 1.0f)
            continue;

        shader->setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty())
        {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            shader->setInt("bone_count", bone_count);
            shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else
        {
            shader->setInt("bone_count", 0);
        }

        shader->setMatrix("model", 1, render_node->model_matrix);

        auto& material = render_node->material;

        shader->setFloat3("diffuse_factor", material.diffuse_factor);
        shader->setFloat3("specular_factor", material.specular_factor);
        shader->setFloat("shininess", material.shininess);
        shader->setTexture("material.diffuse_map", 0, material.diffuse_map);
        shader->setTexture("material.specular_map", 1, material.specular_map);
        shader->setTexture("material.normal_map", 2, material.normal_map);

        shader->setFloat3("cameraPos", context.renderSourceData().camera_position);

        shader->setFloat3("directionalLight.direction", light_direction);
        shader->setFloat4("directionalLight.color", light_color);

        shader->setFloat("alpha", material.alpha);

        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
    shader->stop_using();

    m_rhi->setBlend(false);
    m_rhi->setDepthMask(true);
}
