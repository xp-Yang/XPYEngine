#include "SkyBoxPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

SkyBoxPass::SkyBoxPass()
{
    m_type = RenderPass::Type::SkyBox;
    init();
}

void SkyBoxPass::init()
{
}

void SkyBoxPass::draw(RenderPassContext& context)
{
    RhiFrameBuffer* target_framebuffer = context.readWriteFrameBuffer(RGSlot::Target);
    if (!target_framebuffer)
        return;
    target_framebuffer->bind();

    m_rhi->setFrontFaceCW(true);

    static RenderShaderObject* skybox_shader = RenderShaderObject::getShaderObject(ShaderType::SkyboxShader);
    const auto& render_skybox_sub_mesh_data = m_render_source_data->render_skybox_node.mesh;
    skybox_shader->start_using();
    skybox_shader->setMatrix("model", 1, Mat4(1.0f));
    Mat4 view_without_translation = Mat4(Mat3(m_render_source_data->view_matrix));
    skybox_shader->setMatrix("view", 1, view_without_translation);
    skybox_shader->setMatrix("projection", 1, m_render_source_data->proj_matrix);
    skybox_shader->setCubeTexture("skybox", 0, m_render_source_data->render_skybox_node.skybox_cube_map);
    m_rhi->drawIndexed(render_skybox_sub_mesh_data->getVAO(), render_skybox_sub_mesh_data->indicesCount());
    skybox_shader->stop_using();

    m_rhi->setFrontFaceCW(false);
}
