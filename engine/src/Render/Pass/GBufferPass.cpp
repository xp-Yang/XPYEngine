#include "GBufferPass.hpp"

GBufferPass::GBufferPass()
{
    m_type = RenderPass::Type::GBuffer;
    init();
}

void GBufferPass::init()
{
    RhiTexture* color_texture0 = m_rhi->newTexture(RhiTexture::Format::RGBA16F, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    RhiTexture* color_texture1 = m_rhi->newTexture(RhiTexture::Format::RGBA16F, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    RhiTexture* color_texture2 = m_rhi->newTexture(RhiTexture::Format::RGBA16F, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    RhiTexture* color_texture3 = m_rhi->newTexture(RhiTexture::Format::RGBA16F, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    RhiTexture* color_texture4 = m_rhi->newTexture(RhiTexture::Format::RGBA16F, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    RhiTexture* color_texture5 = m_rhi->newTexture(RhiTexture::Format::RGBA16F, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    RhiTexture* depth_texture = m_rhi->newTexture(RhiTexture::Format::DEPTH, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    color_texture0->create();
    color_texture1->create();
    color_texture2->create();
    color_texture3->create();
    color_texture4->create();
    color_texture5->create();
    depth_texture->create();
    RhiAttachment color_attachment = RhiAttachment(color_texture0);
    RhiAttachment depth_ttachment = RhiAttachment(depth_texture);
    RhiFrameBuffer* fb = m_rhi->newFrameBuffer(color_attachment, Vec2(DEFAULT_RENDER_RESOLUTION_X, DEFAULT_RENDER_RESOLUTION_Y));
    fb->setColorAttachments({ color_texture0 , color_texture1 , color_texture2 , color_texture3 , color_texture4 ,
        color_texture5 });
    fb->setDepthAttachment(depth_ttachment);
    fb->create();
    m_framebuffer = std::unique_ptr<RhiFrameBuffer>(fb);
}

void GBufferPass::draw()
{
    m_framebuffer->bind();
    m_framebuffer->clear();

    static RenderShaderObject* g_pbr_shader = RenderShaderObject::getShaderObject(ShaderType::GBufferShader);
    static RenderShaderObject* g_phong_shader = RenderShaderObject::getShaderObject(ShaderType::GBufferPhongShader);
    RenderShaderObject* g_shader = m_pbr ? g_pbr_shader : g_phong_shader;
    g_shader->start_using();
    g_shader->setMatrix("view", 1, m_render_source_data->view_matrix);
    g_shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
    for (const auto& pair : m_render_source_data->render_mesh_nodes) {
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
            //g_shader->setFloat3("albedo", Vec3(0.25f));
            //g_shader->setFloat("metallic", 0.0f);
            //g_shader->setFloat("roughness", 1.0f);
            //g_shader->setFloat("ao", 0.0f);

            g_shader->setTexture("albedo_map", 0, material.albedo_map);
            g_shader->setTexture("metallic_map", 1, material.metallic_map);
            g_shader->setTexture("roughness_map", 2, material.roughness_map);
            g_shader->setTexture("ao_map", 3, material.ao_map);
        }
        else {
            g_shader->setTexture("diffuse_map", 0, material.diffuse_map);
            g_shader->setTexture("specular_map", 1, material.specular_map);
        }
        m_rhi->drawIndexed(render_node->mesh.getVAO(), render_node->source_index_count, render_node->source_index_offset);
    }
    g_shader->stop_using();

    m_framebuffer->unBind();
}

void GBufferPass::enablePBR(bool enable)
{
    m_pbr = enable;
}
