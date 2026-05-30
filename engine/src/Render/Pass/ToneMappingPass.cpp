#include "ToneMappingPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

ToneMappingPass::ToneMappingPass()
{
    m_type = RenderPass::Type::ToneMapping;
}

void ToneMappingPass::draw(RenderPassContext& context)
{
    RhiTexture* scene_color = context.texture(slot("inColor"));
    RhiFrameBuffer* output_fbo = context.frameBuffer(slot("outColor"));
    if (!scene_color || !output_fbo)
        return;

    auto* quad_layout = context.builtinResources().screen_quad->vertexLayout();
    const int quad_indices = static_cast<int>(context.builtinResources().screen_quad->indicesCount());

    m_command_buffer->beginPass(output_fbo, Color4(0.f, 0.f, 0.f, 1.f));

    RenderPipelineState state;
    state.depthTest = false;
    state.depthWrite = false;
    m_command_buffer->setGraphicsPipeline(
        RenderPipelineLibrary::graphicsPipeline(ShaderType::ToneMappingShader, state));

    ShaderResourceBindings bindings;
    bindings.setTexture("sceneColor", 0, scene_color);
    bindings.setFloat("exposure", m_exposure);
    m_command_buffer->setShaderResources(&bindings);

    m_command_buffer->setVertexInput(quad_layout);
    m_command_buffer->drawIndexed(quad_indices);
    m_command_buffer->endPass();
}
