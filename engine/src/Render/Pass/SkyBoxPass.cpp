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
    RhiFrameBuffer* target_framebuffer = context.frameBuffer(slot("outColor"));
    if (!target_framebuffer)
        return;
    m_command_buffer->beginPass(target_framebuffer, Color4(0.f, 0.f, 0.f, 1.f), 1.0f, 0, false, false);

    const auto& render_skybox_sub_mesh_data = context.renderScene().skybox().mesh;
    RenderPipelineState state;
    state.frontFace = RhiGraphicsPipeline::CW;
    m_command_buffer->setGraphicsPipeline(RenderPipelineLibrary::graphicsPipeline(ShaderType::SkyboxShader, state));
    ShaderResourceBindings bindings;
    bindings.setMatrix("model", 1, Mat4(1.0f));
    Mat4 view_without_translation = Mat4(Mat3(context.frameData().view_matrix));
    bindings.setMatrix("view", 1, view_without_translation);
    bindings.setMatrix("projection", 1, context.frameData().proj_matrix);
    bindings.setCubeTexture("skybox", 0, context.renderScene().skybox().skyboxCubeMap());
    m_command_buffer->setShaderResources(&bindings);
    m_command_buffer->setVertexInput(render_skybox_sub_mesh_data->vertexLayout());
    m_command_buffer->drawIndexed(static_cast<int>(render_skybox_sub_mesh_data->indicesCount()));
    m_command_buffer->endPass();
}
