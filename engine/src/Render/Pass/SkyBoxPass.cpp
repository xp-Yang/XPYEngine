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
    m_command_buffer->beginPass(target_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    static RenderShaderObject* skybox_shader = RenderShaderObject::getShaderObject(ShaderType::SkyboxShader);
    const auto& render_skybox_sub_mesh_data = context.renderSourceData().render_skybox_node.mesh;
    RenderPipelineState state;
    state.frontFace = RhiGraphicsPipeline::CW;
    m_command_buffer->setGraphicsPipeline(skybox_shader->graphicsPipeline(state));
    skybox_shader->setMatrix("model", 1, Mat4(1.0f));
    Mat4 view_without_translation = Mat4(Mat3(context.renderSourceData().view_matrix));
    skybox_shader->setMatrix("view", 1, view_without_translation);
    skybox_shader->setMatrix("projection", 1, context.renderSourceData().proj_matrix);
    skybox_shader->setCubeTexture("skybox", 0, context.renderSourceData().render_skybox_node.skybox_cube_map);
    m_command_buffer->setVertexInput(render_skybox_sub_mesh_data->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(render_skybox_sub_mesh_data->indicesCount()));
    m_command_buffer->endPass();
}
