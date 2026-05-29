#include "SSAOPass.hpp"
#include "Render/Graph/RenderPassContext.hpp"

#include <random>

SSAOPass::SSAOPass()
{
    m_type = RenderPass::Type::SSAO;
    generateKernel();
}

SSAOPass::~SSAOPass()
{
    destroyRawBuffer();
}

void SSAOPass::generateKernel()
{
    std::uniform_real_distribution<float> random_floats(0.0f, 1.0f);
    std::default_random_engine generator(1337u);

    const int kKernelSize = 64;
    m_kernel.reserve(kKernelSize);
    for (int i = 0; i < kKernelSize; ++i)
    {
        // 切线空间半球（z>=0）方向上的随机向量。
        Vec3 sample(
            random_floats(generator) * 2.0f - 1.0f,
            random_floats(generator) * 2.0f - 1.0f,
            random_floats(generator));
        sample = glm::normalize(sample);
        sample *= random_floats(generator);

        // 让样本向原点聚集（近处遮蔽更密集）。
        float scale = static_cast<float>(i) / static_cast<float>(kKernelSize);
        scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
        sample *= scale;

        m_kernel.push_back(sample);
    }
}

void SSAOPass::ensureRawBuffer(const Vec2& size)
{
    if (m_current_size == size && m_raw_framebuffer)
        return;

    destroyRawBuffer();
    m_current_size = size;

    m_raw_texture = m_rhi->newTexture(RhiTexture::Format::R8, size, 1);
    m_raw_texture->create();

    RhiFrameBuffer* fb = m_rhi->newFrameBuffer(RhiAttachment(), size, 1);
    std::array<RhiAttachment, 8> attachments{};
    attachments[0] = RhiAttachment(m_raw_texture, 0, 0, false);
    fb->setColorAttachments(attachments);
    fb->create();
    m_raw_framebuffer.reset(fb);
}

void SSAOPass::destroyRawBuffer()
{
    if (m_raw_framebuffer)
        m_raw_framebuffer->destroyGPU();
    m_raw_framebuffer.reset();

    if (m_raw_texture)
    {
        m_raw_texture->destroy();
        delete m_raw_texture;
        m_raw_texture = nullptr;
    }

    m_current_size = Vec2(0.f, 0.f);
}

void SSAOPass::draw(RenderPassContext& context)
{
    RhiTexture* g_position = context.texture(RGResource::GBufferPosition);
    RhiTexture* g_normal = context.texture(RGResource::GBufferNormal);
    RhiFrameBuffer* result_fbo = context.frameBufferOfTarget(RGTarget::Main);
    if (!g_position || !g_normal || !result_fbo)
        return;

    const Vec2 size = g_position->pixelSize();
    ensureRawBuffer(size);
    if (!m_raw_framebuffer)
        return;

    auto* quad_layout = context.builtinResources().screen_quad->vertexLayout();
    const int quad_indices = static_cast<int>(context.builtinResources().screen_quad->indicesCount());

    // ---- 阶段 1：SSAO 计算 -> 私有 raw FBO ----
    {
        m_command_buffer->beginPass(m_raw_framebuffer.get(), Color4(1.f, 1.f, 1.f, 1.f));

        RenderPipelineState state;
        state.depthTest = false;
        state.depthWrite = false;
        m_command_buffer->setGraphicsPipeline(
            RenderPipelineLibrary::graphicsPipeline(ShaderType::SSAOShader, state));

        ShaderResourceBindings bindings;
        bindings.setTexture("gPosition", 0, g_position);
        bindings.setTexture("gNormal", 1, g_normal);
        bindings.setMatrix("view", 1, context.frameData().view_matrix);
        bindings.setMatrix("projection", 1, context.frameData().proj_matrix);
        for (size_t i = 0; i < m_kernel.size(); ++i)
        {
            std::string name = std::string("samples[") + std::to_string(i) + "]";
            bindings.setFloat3(name, m_kernel[i]);
        }
        bindings.setInt("kernelSize", static_cast<int>(m_kernel.size()));
        bindings.setFloat("radius", m_params.radius);
        bindings.setFloat("bias", m_params.bias);
        bindings.setFloat("power", m_params.power);
        bindings.setFloat2("noiseScale", size);
        m_command_buffer->setShaderResources(&bindings);

        m_command_buffer->setVertexInput(quad_layout);
        m_command_buffer->drawIndexed(quad_indices);
        m_command_buffer->endPass();
    }

    // ---- 阶段 2：盒式模糊 raw -> 图资源 SSAO.Result ----
    {
        m_command_buffer->beginPass(result_fbo, Color4(1.f, 1.f, 1.f, 1.f));

        RenderPipelineState state;
        state.depthTest = false;
        state.depthWrite = false;
        m_command_buffer->setGraphicsPipeline(
            RenderPipelineLibrary::graphicsPipeline(ShaderType::SSAOBlurShader, state));

        ShaderResourceBindings bindings;
        bindings.setTexture("ssaoInput", 0, m_raw_texture);
        bindings.setFloat2("texelSize", Vec2(1.0f / size.x, 1.0f / size.y));
        m_command_buffer->setShaderResources(&bindings);

        m_command_buffer->setVertexInput(quad_layout);
        m_command_buffer->drawIndexed(quad_indices);
        m_command_buffer->endPass();
    }
}
