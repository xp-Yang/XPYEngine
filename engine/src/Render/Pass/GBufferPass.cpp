#include "GBufferPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

GBufferPass::GBufferPass()
{
    m_type = RenderPass::Type::GBuffer;
}

void GBufferPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* framebuffer = context.targetFrameBuffer();
    if (!framebuffer)
        return;
    framebuffer->bind();
    framebuffer->clear();

    static RenderShaderObject* g_pbr_shader = RenderShaderObject::getShaderObject(ShaderType::GBufferShader);
    static RenderShaderObject* g_phong_shader = RenderShaderObject::getShaderObject(ShaderType::GBufferPhongShader);
    RenderShaderObject* g_shader = m_pbr ? g_pbr_shader : g_phong_shader;
    g_shader->start_using();
    g_shader->setMatrix("view", 1, context.renderSourceData().view_matrix);
    g_shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    for (const auto& pair : context.renderSourceData().render_mesh_nodes) {
        const auto& render_node = pair.second;
        if (render_node->material.alpha != 1.0f)
            continue;

        g_shader->setBool("useSkinning", render_node->use_skinning);
        if (render_node->use_skinning && !render_node->bone_matrices.empty())
        {
            int bone_count = std::min((int)render_node->bone_matrices.size(), MAX_BONE_PALETTE_SIZE);
            g_shader->setInt("bone_count", bone_count);
            g_shader->setMatrix("bones[0]", bone_count, render_node->bone_matrices[0]);
        }
        else
        {
            g_shader->setInt("bone_count", 0);
        }

        g_shader->setMatrix("model", 1, render_node->model_matrix);
        auto& material = render_node->material;
        if (m_pbr) {
            g_shader->setFloat3("base_color_factor", material.base_color_factor);
            g_shader->setFloat("metallic_factor", material.metallic_factor);
            g_shader->setFloat("roughness_factor", material.roughness_factor);
            g_shader->setFloat("ao_factor", material.ao_factor);
            g_shader->setTexture("albedo_map", 0, material.albedo_map);
            g_shader->setTexture("metallic_map", 1, material.metallic_map);
            g_shader->setTexture("roughness_map", 2, material.roughness_map);
            g_shader->setTexture("ao_map", 3, material.ao_map);
        }
        else {
            g_shader->setFloat3("diffuse_factor", material.diffuse_factor);
            g_shader->setFloat3("specular_factor", material.specular_factor);
            g_shader->setFloat("shininess", material.shininess);
            g_shader->setTexture("diffuse_map", 0, material.diffuse_map);
            g_shader->setTexture("specular_map", 1, material.specular_map);
        }
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
    g_shader->stop_using();

    framebuffer->unBind();
}

void GBufferPass::enablePBR(bool enable)
{
    m_pbr = enable;
}
