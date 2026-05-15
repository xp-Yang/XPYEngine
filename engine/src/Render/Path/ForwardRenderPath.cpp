#include "ForwardRenderPath.hpp"

#include "../Pass/ShadowPass.hpp"
#include "../Pass/MeshForwardLightingPass.hpp"
#include "../Pass/PickingPass.hpp"
#include "../Pass/SkyBoxPass.hpp"
#include "../Pass/OutlinePass.hpp"
#include "../Pass/CombinePass.hpp"

#include "Render/Graph/RenderGraphDumper.hpp"
#include "../RenderSystem.hpp"

ForwardRenderPath::ForwardRenderPath(RenderSystem *render_system)
{
    m_render_passes[RenderPass::Type::Picking] = std::make_unique<PickingPass>();
    m_render_passes[RenderPass::Type::SkyBox] = std::make_unique<SkyBoxPass>();
    m_render_passes[RenderPass::Type::Shadow] = std::make_unique<ShadowPass>();
    m_render_passes[RenderPass::Type::Forward] = std::make_unique<MeshForwardLightingPass>();
    m_render_passes[RenderPass::Type::Outline] = std::make_unique<OutlinePass>();
    m_render_passes[RenderPass::Type::Combined] = std::make_unique<CombinePass>();

    ref_render_system = render_system;
}

void ForwardRenderPath::resizeRenderTargets(const Vec2& pixel_size)
{
    m_render_graph.setFrameSize(pixel_size);
}

unsigned int ForwardRenderPath::getPickingFBO()
{
    RhiFrameBuffer* framebuffer = m_render_graph.frameBuffer(RGResource::PickingColor);
    return framebuffer ? framebuffer->id() : 0;
}

void ForwardRenderPath::render()
{
    const auto& render_params = ref_render_system->renderParams();
    auto pass = [this](RenderPass::Type type) -> RenderPass*
    {
        return m_render_passes.at(type).get();
    };

    // TODO samples更改了，重新初始化整个path，重新创建相关的RenderPass。
    // configShadowMapSamples(render_params.shadow_map_sample_count);
    // configSamples(render_params.msaa_sample_count);

    m_render_graph.reset();

    m_render_graph.addPass("Picking", RenderPass::Type::Picking, pass(RenderPass::Type::Picking))
        .color(RGResource::PickingColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::PickingDepth, RhiTexture::Format::DEPTH);

    m_render_graph.addPass("Shadow", RenderPass::Type::Shadow, pass(RenderPass::Type::Shadow))
        .setEnabled(render_params.shadow_params.enable)
        .setDisabledExecution(RenderGraph::DisabledExecution::Clear)
        .color(RGResource::ShadowDirectionalColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::ShadowDirectionalDepth, RhiTexture::Format::DEPTH)
        .cubeDepthTarget(RGTarget::ShadowPointDepth);

    m_render_graph.addPass("ForwardLighting", RenderPass::Type::Forward, pass(RenderPass::Type::Forward))
        .read(RGResource::ShadowDirectionalDepth)
        .color(RGResource::SceneColor, RhiTexture::Format::RGB8, 0, 4)
        .depthStencil(RGResource::SceneDepth, RhiTexture::Format::DEPTH24STENCIL8, 4)
        .setSetup([this, &render_params](RenderPass& render_pass)
        {
            auto& main_pass = static_cast<MeshForwardLightingPass&>(render_pass);
            main_pass.enablePBR(render_params.material_model == MaterialModel::PBR);
            main_pass.enableReflection(render_params.effect_params.reflection);
            main_pass.setCubeMaps(m_render_graph.cubeDepthTextures(RenderPass::Type::Shadow, RGTarget::ShadowPointDepth));
        });

    m_render_graph.addPass("SkyBox", RenderPass::Type::SkyBox, pass(RenderPass::Type::SkyBox))
        .setEnabled(render_params.effect_params.skybox)
        .readWriteAs(RGSlot::Target, RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth);

    m_render_graph.addPass("Outline", RenderPass::Type::Outline, pass(RenderPass::Type::Outline))
        .readWriteAs(RGSlot::Target, RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth)
        .target(RGTarget::OutlineMask)
        .color(RGResource::OutlineMaskColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::OutlineMaskDepth, RhiTexture::Format::DEPTH);

    m_render_graph.addPass("Combined", RenderPass::Type::Combined, pass(RenderPass::Type::Combined))
        .readAs(RGSlot::Source, RGResource::SceneColor)
        .read(RGResource::SceneDepth)
        .color(RGResource::FinalColor, RhiTexture::Format::RGB8)
        .depth(RGResource::FinalDepth, RhiTexture::Format::DEPTH)
        .backbuffer(RGTarget::Backbuffer)
        .setSetup([&render_params](RenderPass& render_pass)
        {
            static_cast<CombinePass&>(render_pass).enableFXAA(render_params.post_processing_params.fxaa);
        });

    m_render_graph.markOutput(RGResource::PickingColor);
    m_render_graph.markOutput(RGResource::FinalColor);

    m_render_graph.compile();
    m_render_graph.execute();
}

RhiTexture* ForwardRenderPath::renderGraphTexture(const std::string& resource_name)
{
    return m_render_graph.texture(resource_name);
}

std::vector<std::string> ForwardRenderPath::renderGraphResourceNames() const
{
    return RenderGraphDumper(m_render_graph).resourceNames();
}

std::string ForwardRenderPath::renderGraphDebugDump() const
{
    return RenderGraphDumper(m_render_graph).graph();
}

std::string ForwardRenderPath::renderGraphExecutionDump() const
{
    return RenderGraphDumper(m_render_graph).executionOrder();
}
