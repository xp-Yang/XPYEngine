#include "DeferredRenderPath.hpp"

#include "../Pass/ZPrePass.hpp"
#include "../Pass/ShadowPass.hpp"
#include "../Pass/WireFramePass.hpp"
#include "../Pass/CheckerBoardPass.hpp"
#include "../Pass/NormalPass.hpp"
#include "../Pass/GBufferPass.hpp"
#include "../Pass/DeferredLightingPass.hpp"
#include "../Pass/TransparentPass.hpp"
#include "../Pass/SkyBoxPass.hpp"
#include "../Pass/BloomPass.hpp"
#include "../Pass/PickingPass.hpp"
#include "../Pass/OutlinePass.hpp"
#include "../Pass/CombinePass.hpp"

#include "Render/Graph/RenderGraphDumper.hpp"
#include "../RenderSystem.hpp"

DeferredRenderPath::DeferredRenderPath(RenderSystem* render_system)
{
    m_render_passes[RenderPass::Type::ZPre] = std::make_unique<ZPrePass>();
    m_render_passes[RenderPass::Type::Picking] = std::make_unique<PickingPass>();
    m_render_passes[RenderPass::Type::SkyBox] = std::make_unique<SkyBoxPass>();
    m_render_passes[RenderPass::Type::Shadow] = std::make_unique<ShadowPass>();
    m_render_passes[RenderPass::Type::GBuffer] = std::make_unique<GBufferPass>();
    m_render_passes[RenderPass::Type::DeferredLighting] = std::make_unique<DeferredLightingPass>();
    m_render_passes[RenderPass::Type::Transparent] = std::make_unique<TransparentPass>();
    m_render_passes[RenderPass::Type::Bloom] = std::make_unique<BloomPass>();
    m_render_passes[RenderPass::Type::Outline] = std::make_unique<OutlinePass>();
    m_render_passes[RenderPass::Type::Combined] = std::make_unique<CombinePass>();
    m_render_passes[RenderPass::Type::WireFrame] = std::make_unique<WireFramePass>();
    m_render_passes[RenderPass::Type::CheckerBoard] = std::make_unique<CheckerBoardPass>();
    m_render_passes[RenderPass::Type::Normal] = std::make_unique<NormalPass>();

    ref_render_system = render_system;
}

void DeferredRenderPath::resizeRenderTargets(const Vec2& pixel_size)
{
    m_render_graph.setFrameSize(pixel_size);
}

unsigned int DeferredRenderPath::getPickingFBO()
{
    RhiFrameBuffer* framebuffer = m_render_graph.frameBuffer(RGResource::PickingColor);
    return framebuffer ? framebuffer->id() : 0;
}

void DeferredRenderPath::render()
{
    const auto& render_params = ref_render_system->renderParams();
    const bool checkerboard_enabled = render_params.effect_params.checkerboard;
    const bool bloom_used = render_params.post_processing_params.bloom && !checkerboard_enabled;
    auto pass = [this](RenderPass::Type type) -> RenderPass*
    {
        return m_render_passes.at(type).get();
    };

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

    auto& gbuffer_node = m_render_graph.addPass("GBuffer", RenderPass::Type::GBuffer, pass(RenderPass::Type::GBuffer))
        .color(RGResource::GBufferPosition, RhiTexture::Format::RGBA16F, 0)
        .color(RGResource::GBufferNormal, RhiTexture::Format::RGBA16F, 1)
        .depth(RGResource::GBufferDepth, RhiTexture::Format::DEPTH)
        .setSetup([&render_params](RenderPass& render_pass)
        {
            static_cast<GBufferPass&>(render_pass).enablePBR(render_params.material_model == MaterialModel::PBR);
        });
    if (render_params.material_model == MaterialModel::PBR)
    {
        gbuffer_node
            .color(RGResource::GBufferAlbedo, RhiTexture::Format::RGBA16F, 2)
            .color(RGResource::GBufferMetallic, RhiTexture::Format::RGBA16F, 3)
            .color(RGResource::GBufferRoughness, RhiTexture::Format::RGBA16F, 4)
            .color(RGResource::GBufferAO, RhiTexture::Format::RGBA16F, 5);
    }
    else
    {
        gbuffer_node
            .color(RGResource::GBufferDiffuse, RhiTexture::Format::RGBA16F, 2)
            .color(RGResource::GBufferSpecular, RhiTexture::Format::RGBA16F, 3);
    }

    auto& lighting_node = m_render_graph.addPass("DeferredLighting", RenderPass::Type::DeferredLighting, pass(RenderPass::Type::DeferredLighting))
        .readAs(RGSlot::GBuffer, RGResource::GBufferPosition)
        .read(RGResource::GBufferNormal)
        .read(RGResource::ShadowDirectionalDepth)
        .color(RGResource::SceneColor, RhiTexture::Format::RGBA16F)
        .depth(RGResource::SceneDepth, RhiTexture::Format::DEPTH)
        .setSetup([this, &render_params](RenderPass& render_pass)
        {
            auto& lighting_pass = static_cast<DeferredLightingPass&>(render_pass);
            lighting_pass.enablePBR(render_params.material_model == MaterialModel::PBR);
            lighting_pass.setCubeMaps(m_render_graph.cubeDepthTextures(RenderPass::Type::Shadow, RGTarget::ShadowPointDepth));
        });
    if (render_params.material_model == MaterialModel::PBR)
    {
        lighting_node
            .read(RGResource::GBufferAlbedo)
            .read(RGResource::GBufferMetallic)
            .read(RGResource::GBufferRoughness)
            .read(RGResource::GBufferAO);
    }
    else
    {
        lighting_node
            .read(RGResource::GBufferDiffuse)
            .read(RGResource::GBufferSpecular);
    }

    m_render_graph.addPass("SkyBox", RenderPass::Type::SkyBox, pass(RenderPass::Type::SkyBox))
        .setEnabled(render_params.effect_params.skybox)
        .readWriteAs(RGSlot::Target, RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth);

    m_render_graph.addPass("Transparent", RenderPass::Type::Transparent, pass(RenderPass::Type::Transparent))
        .readWriteAs(RGSlot::Target, RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth);

    m_render_graph.addPass("Bloom", RenderPass::Type::Bloom, pass(RenderPass::Type::Bloom))
        .setEnabled(bloom_used)
        .setDisabledExecution(RenderGraph::DisabledExecution::Clear)
        .readAs(RGSlot::Source, RGResource::SceneColor)
        .color(RGResource::BloomColor, RhiTexture::Format::RGB16F)
        .target(RGTarget::BloomPingPong)
        .color(RGResource::BloomPingPongColor, RhiTexture::Format::RGB16F);

    m_render_graph.addPass("CheckerBoard", RenderPass::Type::CheckerBoard, pass(RenderPass::Type::CheckerBoard))
        .setEnabled(checkerboard_enabled)
        .color(RGResource::CheckerBoardColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::CheckerBoardDepth, RhiTexture::Format::DEPTH);

    const std::string main_color = checkerboard_enabled ? RGResource::CheckerBoardColor : RGResource::SceneColor;
    const std::string main_depth = checkerboard_enabled ? RGResource::CheckerBoardDepth : RGResource::SceneDepth;

    m_render_graph.addPass("Normal", RenderPass::Type::Normal, pass(RenderPass::Type::Normal))
        .setEnabled(render_params.effect_params.show_normal)
        .readWriteAs(RGSlot::Target, main_color)
        .readWrite(main_depth);

    m_render_graph.addPass("WireFrame", RenderPass::Type::WireFrame, pass(RenderPass::Type::WireFrame))
        .setEnabled(render_params.effect_params.wireframe)
        .readWriteAs(RGSlot::Target, main_color)
        .readWrite(main_depth);

    m_render_graph.addPass("Outline", RenderPass::Type::Outline, pass(RenderPass::Type::Outline))
        .readWriteAs(RGSlot::Target, main_color)
        .readWrite(main_depth)
        .target(RGTarget::OutlineMask)
        .color(RGResource::OutlineMaskColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::OutlineMaskDepth, RhiTexture::Format::DEPTH);

    auto& combine_node = m_render_graph.addPass("Combined", RenderPass::Type::Combined, pass(RenderPass::Type::Combined))
        .readAs(RGSlot::Source, main_color)
        .read(main_depth)
        .color(RGResource::FinalColor, RhiTexture::Format::RGB8)
        .depth(RGResource::FinalDepth, RhiTexture::Format::DEPTH)
        .backbuffer(RGTarget::Backbuffer)
        .setSetup([&render_params](RenderPass& render_pass)
        {
            static_cast<CombinePass&>(render_pass).enableFXAA(render_params.post_processing_params.fxaa);
        });

    if (bloom_used)
        combine_node.readAs(RGSlot::Bloom, RGResource::BloomColor);

    m_render_graph.markOutput(RGResource::PickingColor);
    if (!bloom_used)
        m_render_graph.markOutput(RGResource::BloomColor);
    m_render_graph.markOutput(RGResource::FinalColor);

    m_render_graph.compile();
    m_render_graph.execute();
}

RhiTexture* DeferredRenderPath::renderGraphTexture(const std::string& resource_name)
{
    return m_render_graph.texture(resource_name);
}

std::vector<std::string> DeferredRenderPath::renderGraphResourceNames() const
{
    return RenderGraphDumper(m_render_graph).resourceNames();
}

std::string DeferredRenderPath::renderGraphDebugDump() const
{
    return RenderGraphDumper(m_render_graph).graph();
}

std::string DeferredRenderPath::renderGraphExecutionDump() const
{
    return RenderGraphDumper(m_render_graph).executionOrder();
}
