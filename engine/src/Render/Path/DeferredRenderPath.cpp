#include "DeferredRenderPath.hpp"

#include "../Pass/ZPrePass.hpp"
#include "../Pass/ShadowPass.hpp"
#include "../Pass/WireFramePass.hpp"
#include "../Pass/CheckerBoardPass.hpp"
#include "../Pass/NormalPass.hpp"
#include "../Pass/GBufferPass.hpp"
#include "../Pass/DeferredLightingPass.hpp"
#include "../Pass/SSAOPass.hpp"
#include "../Pass/TransparentPass.hpp"
#include "../Pass/SkyBoxPass.hpp"
#include "../Pass/BloomPass.hpp"
#include "../Pass/PickingPass.hpp"
#include "../Pass/OutlinePass.hpp"
#include "../Pass/FXAAPass.hpp"
#include "../Pass/ToneMappingPass.hpp"
#include "../Pass/FinalPass.hpp"

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
    m_render_passes[RenderPass::Type::SSAO] = std::make_unique<SSAOPass>();
    m_render_passes[RenderPass::Type::Transparent] = std::make_unique<TransparentPass>();
    m_render_passes[RenderPass::Type::Bloom] = std::make_unique<BloomPass>();
    m_render_passes[RenderPass::Type::Outline] = std::make_unique<OutlinePass>();
    m_render_passes[RenderPass::Type::FXAA] = std::make_unique<FXAAPass>();
    m_render_passes[RenderPass::Type::ToneMapping] = std::make_unique<ToneMappingPass>();
    m_render_passes[RenderPass::Type::WireFrame] = std::make_unique<WireFramePass>();
    m_render_passes[RenderPass::Type::CheckerBoard] = std::make_unique<CheckerBoardPass>();
    m_render_passes[RenderPass::Type::Normal] = std::make_unique<NormalPass>();
    m_render_passes[RenderPass::Type::Final] = std::make_unique<FinalPass>();

    ref_render_system = render_system;
}

void DeferredRenderPath::resizeRenderTargets(const Vec2& pixel_size)
{
    m_render_graph.setFrameSize(pixel_size);
}

void DeferredRenderPath::render(RenderScene& render_scene, RenderFrameData& frame_data, RenderBuiltinResources& builtin_resources)
{
    const auto& render_params = ref_render_system->renderParams();
    const bool checkerboard_enabled = render_params.effect_params.checkerboard;
    const bool bloom_used = render_params.post_processing_params.bloom && !checkerboard_enabled;
    const bool tone_mapping_used = render_params.post_processing_params.tone_mapping
        && render_params.post_processing_params.hdr
        && !checkerboard_enabled;
    auto pass = [this](RenderPass::Type type) -> RenderPass*
    {
        return m_render_passes.at(type).get();
    };

    m_render_graph.reset();

    // --- Picking ---
    m_render_graph.addPass(RenderPass::Type::Picking, pass(RenderPass::Type::Picking))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::PickingColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::PickingDepth, RhiTexture::Format::DEPTH)
        .setSetup([](RenderPass& p)
        {
            p.bindSlot("outTarget", RGTarget::Main);
        });

    // --- Shadow ---
    m_render_graph.addPass(RenderPass::Type::Shadow, pass(RenderPass::Type::Shadow))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .setEnabled(render_params.shadow_params.enable)
        .setDisabledExecution(RGDisabledExecution::Clear)
        .color(RGResource::ShadowDirectionalColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::ShadowDirectionalDepth, RhiTexture::Format::DEPTH)
        .target(RGTarget::ShadowPointDepth, RenderTargetType::CubeDepth)
        .setSetup([](RenderPass& p)
        {
            p.bindSlot("outTarget", RGTarget::Main);
        });

    // --- GBuffer ---
    auto& gbuffer_node = m_render_graph.addPass(RenderPass::Type::GBuffer, pass(RenderPass::Type::GBuffer))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::GBufferPosition, RhiTexture::Format::RGBA16F, 0)
        .color(RGResource::GBufferNormal, RhiTexture::Format::RGBA16F, 1)
        .depth(RGResource::GBufferDepth, RhiTexture::Format::DEPTH)
        .setSetup([&render_params](RenderPass& p)
        {
            static_cast<GBufferPass&>(p).enablePBR(render_params.material_model == MaterialModel::PBR);
            p.bindSlot("outTarget", RGTarget::Main);
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

    // --- SSAO ---
    const bool ssao_used = render_params.ssao.enable && render_params.material_model == MaterialModel::PBR;
    if (ssao_used)
    {
        m_render_graph.addPass(RenderPass::Type::SSAO, pass(RenderPass::Type::SSAO))
            .read(RGResource::GBufferPosition)
            .read(RGResource::GBufferNormal)
            .target(RGTarget::Main, RenderTargetType::FrameBuffer)
            .color(RGResource::SSAOResult, RhiTexture::Format::R8, 0)
            .setSetup([&render_params](RenderPass& p)
            {
                static_cast<SSAOPass&>(p).setParams(render_params.ssao);
                p.bindSlot("inPosition", RGResource::GBufferPosition);
                p.bindSlot("inNormal", RGResource::GBufferNormal);
                p.bindSlot("outTarget", RGTarget::Main);
            });
    }

    // --- Deferred Lighting ---
    auto& lighting_node = m_render_graph.addPass(RenderPass::Type::DeferredLighting, pass(RenderPass::Type::DeferredLighting))
        .read(RGResource::GBufferPosition)
        .read(RGResource::GBufferNormal)
        .read(RGResource::ShadowDirectionalDepth)
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::SceneColor, RhiTexture::Format::RGBA16F)
        .depth(RGResource::SceneDepth, RhiTexture::Format::DEPTH)
        .setSetup([this, &render_params, ssao_used](RenderPass& p)
        {
            auto& lighting_pass = static_cast<DeferredLightingPass&>(p);
            lighting_pass.enablePBR(render_params.material_model == MaterialModel::PBR);
            lighting_pass.enableIBL(render_params.ibl.enable);
            lighting_pass.enableSSAO(ssao_used);
            p.bindSlot("outTarget", RGTarget::Main);
            p.bindSlot("inGBufferPosition", RGResource::GBufferPosition);
            p.bindSlot("inGBufferNormal", RGResource::GBufferNormal);
            p.bindSlot("inShadowDepth", RGResource::ShadowDirectionalDepth);
            p.bindSlot("inGBufferDepthFBO", RGResource::GBufferDepth);
            if (render_params.material_model == MaterialModel::PBR)
            {
                p.bindSlot("inGBufferAlbedo", RGResource::GBufferAlbedo);
                p.bindSlot("inGBufferMetallic", RGResource::GBufferMetallic);
                p.bindSlot("inGBufferRoughness", RGResource::GBufferRoughness);
                p.bindSlot("inGBufferAO", RGResource::GBufferAO);
            }
            else
            {
                p.bindSlot("inGBufferDiffuse", RGResource::GBufferDiffuse);
                p.bindSlot("inGBufferSpecular", RGResource::GBufferSpecular);
            }
            if (ssao_used)
                p.bindSlot("inSSAO", RGResource::SSAOResult);
        });
    if (ssao_used)
    {
        lighting_node.read(RGResource::SSAOResult);
    }
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

    // --- In-place modify passes (SkyBox, Transparent, Outline, WireFrame, Normal) ---
    m_render_graph.addPass(RenderPass::Type::SkyBox, pass(RenderPass::Type::SkyBox))
        .setEnabled(render_params.effect_params.skybox)
        .modify(RGResource::SceneColor)
        .modify(RGResource::SceneDepth)
        .setSetup([](RenderPass& p)
        {
            p.bindSlot("outColor", RGResource::SceneColor);
        });

    m_render_graph.addPass(RenderPass::Type::Transparent, pass(RenderPass::Type::Transparent))
        .setEnabled(render_scene.hasTransparent())
        .modify(RGResource::SceneColor)
        .modify(RGResource::SceneDepth)
        .setSetup([](RenderPass& p)
        {
            p.bindSlot("outColor", RGResource::SceneColor);
        });

    m_render_graph.addPass(RenderPass::Type::Outline, pass(RenderPass::Type::Outline))
        .setEnabled(!frame_data.picked_ids.empty())
        .modify(RGResource::SceneColor)
        .modify(RGResource::SceneDepth)
        .target(RGTarget::OutlineMask, RenderTargetType::FrameBuffer)
        .color(RGResource::OutlineMaskColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::OutlineMaskDepth, RhiTexture::Format::DEPTH)
        .setSetup([](RenderPass& p)
        {
            p.bindSlot("outColor", RGResource::SceneColor);
            p.bindSlot("outMaskTarget", RGTarget::OutlineMask);
            p.bindSlot("inMaskColor", RGResource::OutlineMaskColor);
            p.bindSlot("inMaskDepth", RGResource::OutlineMaskDepth);
        });

    m_render_graph.addPass(RenderPass::Type::CheckerBoard, pass(RenderPass::Type::CheckerBoard))
        .setEnabled(checkerboard_enabled)
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::CheckerBoardColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::CheckerBoardDepth, RhiTexture::Format::DEPTH)
        .setSetup([](RenderPass& p)
        {
            p.bindSlot("outTarget", RGTarget::Main);
        });

    m_render_graph.addPass(RenderPass::Type::WireFrame, pass(RenderPass::Type::WireFrame))
        .setEnabled(render_params.effect_params.wireframe)
        .modify(RGResource::SceneColor)
        .modify(RGResource::SceneDepth)
        .setSetup([](RenderPass& p)
        {
            p.bindSlot("outColor", RGResource::SceneColor);
        });

    m_render_graph.addPass(RenderPass::Type::Normal, pass(RenderPass::Type::Normal))
        .setEnabled(render_params.effect_params.show_normal)
        .modify(RGResource::SceneColor)
        .modify(RGResource::SceneDepth)
        .setSetup([](RenderPass& p)
        {
            p.bindSlot("outColor", RGResource::SceneColor);
        });

    // --- Post-processing chain: read(in) + write(out) ---
    RGResourceName currentColor = RGResource::SceneColor;

    if (bloom_used)
    {
        const RGResourceName bloomIn = currentColor;
        const RGResourceName bloomOut = RGResource::BloomColor;
        m_render_graph.addPass(RenderPass::Type::Bloom, pass(RenderPass::Type::Bloom))
            .setEnabled(true)
            .read(bloomIn)
            .target(RGTarget::Main, RenderTargetType::FrameBuffer)
            .color(bloomOut, RhiTexture::Format::RGBA16F)
            .setSetup([&render_params, bloomIn, bloomOut](RenderPass& p)
            {
                auto& bloom_pass = static_cast<BloomPass&>(p);
                const auto& pp = render_params.post_processing_params;
                bloom_pass.setParams({
                    pp.bloom_threshold,
                    pp.bloom_soft_knee,
                    pp.bloom_intensity,
                    pp.bloom_mip_levels
                });
                p.bindSlot("inColor", bloomIn);
                p.bindSlot("outColor", bloomOut);
            });
        currentColor = bloomOut;
    }

    if (render_params.post_processing_params.fxaa)
    {
        const RGResourceName fxaaIn = currentColor;
        const RGResourceName fxaaOut = RGResource::FXAAColor;
        m_render_graph.addPass(RenderPass::Type::FXAA, pass(RenderPass::Type::FXAA))
            .setEnabled(true)
            .read(fxaaIn)
            .target(RGTarget::Main, RenderTargetType::FrameBuffer)
            .color(fxaaOut, RhiTexture::Format::RGBA16F)
            .setSetup([fxaaIn, fxaaOut](RenderPass& p)
            {
                p.bindSlot("inColor", fxaaIn);
                p.bindSlot("outColor", fxaaOut);
            });
        currentColor = fxaaOut;
    }

    if (tone_mapping_used)
    {
        const RGResourceName tmIn = currentColor;
        const RGResourceName tmOut = RGResource::ToneMappingColor;
        m_render_graph.addPass(RenderPass::Type::ToneMapping, pass(RenderPass::Type::ToneMapping))
            .setEnabled(true)
            .read(tmIn)
            .target(RGTarget::Main, RenderTargetType::FrameBuffer)
            .color(tmOut, RhiTexture::Format::RGBA16F)
            .setSetup([&render_params, tmIn, tmOut](RenderPass& p)
            {
                auto& tone_pass = static_cast<ToneMappingPass&>(p);
                tone_pass.setExposure(render_params.post_processing_params.exposure);
                p.bindSlot("inColor", tmIn);
                p.bindSlot("outColor", tmOut);
            });
        currentColor = tmOut;
    }

    // --- Final ---
    RGResourceName beforeFinalColor = checkerboard_enabled ? RGResource::CheckerBoardColor : currentColor;
    RGResourceName beforeFinalDepth = checkerboard_enabled ? RGResource::CheckerBoardDepth : RGResource::SceneDepth;
    m_render_graph.addPass(RenderPass::Type::Final, pass(RenderPass::Type::Final))
        .read(beforeFinalColor)
        .read(beforeFinalDepth)
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::FinalColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::FinalDepth, RhiTexture::Format::DEPTH)
        .target(RGTarget::ScreenFrameBuffer, RenderTargetType::ScreenFrameBuffer)
        .setSetup([&render_params, beforeFinalColor, beforeFinalDepth](RenderPass& p)
        {
            static_cast<FinalPass&>(p).setDrawGrid(render_params.effect_params.grid);
            p.bindSlot("inColor", beforeFinalColor);
            p.bindSlot("inDepth", beforeFinalDepth);
            p.bindSlot("outTarget", RGTarget::Main);
        });

    m_render_graph.markOutput(RGResource::PickingColor);
    m_render_graph.markOutput(RGResource::FinalColor);

    m_render_graph.compile();
    m_render_graph.execute(render_scene, frame_data, builtin_resources);
}

RhiTexture* DeferredRenderPath::renderGraphTextureOf(const std::string& resource_name)
{
    return m_render_graph.textureOf(resource_name);
}

bool DeferredRenderPath::readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4])
{
    return m_render_graph.readPixelRGBAOf(resource_name, x, y, out_rgba);
}

std::vector<std::string> DeferredRenderPath::renderGraphResourceNames() const
{
    return RenderGraphDumper(m_render_graph).resourceNames();
}

std::vector<RenderGraphResourceDebugInfo> DeferredRenderPath::renderGraphResourceDebugInfos() const
{
    return RenderGraphDumper(m_render_graph).resourceInfos();
}

std::string DeferredRenderPath::renderGraphDebugDump() const
{
    return RenderGraphDumper(m_render_graph).graph();
}

std::string DeferredRenderPath::renderGraphExecutionDump() const
{
    return RenderGraphDumper(m_render_graph).executionOrder();
}
