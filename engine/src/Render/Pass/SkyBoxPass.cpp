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
    RhiFrameBuffer* target_framebuffer = context.frameBuffer(RGResource::SceneColor);
    if (!target_framebuffer)
        return;
    target_framebuffer->bind();

    m_rhi->setFrontFaceCW(true);

    static RenderShaderObject* skybox_shader = RenderShaderObject::getShaderObject(ShaderType::SkyboxShader);
    const auto& render_skybox_sub_mesh_data = context.renderSourceData().render_skybox_node.mesh;
    skybox_shader->start_using();
    skybox_shader->setMatrix("model", 1, Mat4(1.0f));
    Mat4 view_without_translation = Mat4(Mat3(context.renderSourceData().view_matrix));
    skybox_shader->setMatrix("view", 1, view_without_translation);
    skybox_shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    skybox_shader->setCubeTexture("skybox", 0, context.renderSourceData().render_skybox_node.skybox_cube_map);
    m_rhi->drawIndexed(render_skybox_sub_mesh_data->getVAO(), render_skybox_sub_mesh_data->indicesCount());
    skybox_shader->stop_using();

    m_rhi->setFrontFaceCW(false);
}
