#include "ForwardRenderPath.hpp"

#include "../Pass/ShadowPass.hpp"
#include "../Pass/MeshForwardLightingPass.hpp"
#include "../Pass/PickingPass.hpp"
#include "../Pass/SkyBoxPass.hpp"
#include "../Pass/OutlinePass.hpp"
#include "../Pass/BloomPass.hpp"
#include "../Pass/FXAAPass.hpp"
#include "../Pass/ToneMappingPass.hpp"
#include "../Pass/FinalPass.hpp"
#include "../Pass/UIPass.hpp"

#include "Render/Graph/RenderGraphDumper.hpp"
#include "../RenderSystem.hpp"

ForwardRenderPath::ForwardRenderPath(RenderSystem *render_system)
{
    m_render_passes[RenderPass::Type::Picking] = std::make_unique<PickingPass>();
    m_render_passes[RenderPass::Type::SkyBox] = std::make_unique<SkyBoxPass>();
    m_render_passes[RenderPass::Type::Shadow] = std::make_unique<ShadowPass>();
    m_render_passes[RenderPass::Type::Forward] = std::make_unique<MeshForwardLightingPass>();
    m_render_passes[RenderPass::Type::Outline] = std::make_unique<OutlinePass>();
    m_render_passes[RenderPass::Type::Bloom] = std::make_unique<BloomPass>();
    m_render_passes[RenderPass::Type::FXAA] = std::make_unique<FXAAPass>();
    m_render_passes[RenderPass::Type::ToneMapping] = std::make_unique<ToneMappingPass>();
    m_render_passes[RenderPass::Type::Final] = std::make_unique<FinalPass>();
    m_render_passes[RenderPass::Type::UI] = std::make_unique<UIPass>();

    ref_render_system = render_system;
}

void ForwardRenderPath::resizeRenderTargets(const Vec2& pixel_size)
{
    m_render_graph.setFrameSize(pixel_size);
    const auto& shadow_params = ref_render_system->renderParams().shadow_params;
    m_render_graph.setShadowTargetSizes(
        shadowDirectionalPixelSize(pixel_size, shadow_params.directional_resolution_scale),
        shadowPointCubeEdge(shadow_params.point_cube_resolution));
}

void ForwardRenderPath::render(RenderScene& render_scene, RenderFrameData& frame_data, RenderBuiltinResources& builtin_resources)
{
    const auto& render_params = ref_render_system->renderParams();
    auto pass = [this](RenderPass::Type type) -> RenderPass*
    {
        return m_render_passes.at(type).get();
    };

    m_render_graph.reset();

    m_render_graph.addPass(RenderPass::Type::Picking, pass(RenderPass::Type::Picking))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::PickingColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::PickingDepth, RhiTexture::Format::DEPTH)
        .setSetup([](RenderPass& p) { p.bindSlot("outTarget", RGTarget::Main); });

    m_render_graph.addPass(RenderPass::Type::Shadow, pass(RenderPass::Type::Shadow))
        .setEnabled(render_params.shadow_params.directional_enable)
        .setDisabledExecution(RGDisabledExecution::Clear)
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::ShadowDirectionalColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::ShadowDirectionalDepth, RhiTexture::Format::DEPTH)
        .target(RGTarget::ShadowPointDepth, RenderTargetType::CubeDepth)
        .setSetup([&render_params](RenderPass& p)
        {
            auto& shadow_pass = static_cast<ShadowPass&>(p);
            shadow_pass.setRenderDirectionalShadows(render_params.shadow_params.directional_enable);
            shadow_pass.setRenderPointShadows(false);
            p.bindSlot("outTarget", RGTarget::Main);
        });

    m_render_graph.addPass(RenderPass::Type::Forward, pass(RenderPass::Type::Forward))
        .read(RGResource::ShadowDirectionalDepth)
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::SceneColor, RhiTexture::Format::RGB8)
        .depthStencil(RGResource::SceneDepth, RhiTexture::Format::DEPTH24STENCIL8)
        .setSetup([this, &render_params](RenderPass& p)
        {
            auto& main_pass = static_cast<MeshForwardLightingPass&>(p);
            main_pass.enablePBR(render_params.material_model == MaterialModel::PBR);
            main_pass.enableReflection(render_params.effect_params.reflection);
            main_pass.enableIBL(render_params.ibl.enable);
            main_pass.enableDirectionalShadow(render_params.shadow_params.directional_enable);
            p.bindSlot("outTarget", RGTarget::Main);
            p.bindSlot("inShadowDepth", RGResource::ShadowDirectionalDepth);
        });

    m_render_graph.addPass(RenderPass::Type::SkyBox, pass(RenderPass::Type::SkyBox))
        .setEnabled(render_params.effect_params.skybox)
        .modify(RGResource::SceneColor)
        .modify(RGResource::SceneDepth)
        .setSetup([](RenderPass& p) { p.bindSlot("outColor", RGResource::SceneColor); });

    m_render_graph.addPass(RenderPass::Type::Outline, pass(RenderPass::Type::Outline))
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

    // --- Post-processing chain: read(in) + write(out) ---
    RGResourceName currentColor = RGResource::SceneColor;
    const bool bloom_used = render_params.post_processing_params.bloom;
    const bool tone_mapping_used = render_params.post_processing_params.tone_mapping
        && render_params.post_processing_params.hdr;
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

    m_render_graph.addPass(RenderPass::Type::Final, pass(RenderPass::Type::Final))
        .read(currentColor)
        .read(RGResource::SceneDepth)
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::FinalColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::FinalDepth, RhiTexture::Format::DEPTH)
        .setSetup([&render_params, currentColor](RenderPass& p)
        {
            static_cast<FinalPass&>(p).setDrawGrid(render_params.effect_params.grid);
            p.bindSlot("inColor", currentColor);
            p.bindSlot("inDepth", RGResource::SceneDepth);
            p.bindSlot("outTarget", RGTarget::Main);
        });

    m_render_graph.addPass(RenderPass::Type::UI, pass(RenderPass::Type::UI))
        .read(RGResource::SceneColor)
        .target(RGTarget::ScreenFrameBuffer, RenderTargetType::ScreenFrameBuffer);

    m_render_graph.markOutput(RGResource::PickingColor);
    m_render_graph.markOutput(RGResource::FinalColor);
    m_render_graph.markOutputPass(RenderPass::Type::UI);

    m_render_graph.compile();
    m_render_graph.execute(render_scene, frame_data, builtin_resources);
}

RhiTexture* ForwardRenderPath::renderGraphTextureOf(const std::string& resource_name)
{
    return m_render_graph.textureOf(resource_name);
}

bool ForwardRenderPath::readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4])
{
    return m_render_graph.readPixelRGBAOf(resource_name, x, y, out_rgba);
}

std::vector<std::string> ForwardRenderPath::renderGraphResourceNames() const
{
    return RenderGraphDumper(m_render_graph).resourceNames();
}

std::vector<RenderGraphResourceDebugInfo> ForwardRenderPath::renderGraphResourceDebugInfos() const
{
    return RenderGraphDumper(m_render_graph).resourceInfos();
}

std::string ForwardRenderPath::renderGraphDebugDump() const
{
    return RenderGraphDumper(m_render_graph).graph();
}

std::string ForwardRenderPath::renderGraphExecutionDump() const
{
    return RenderGraphDumper(m_render_graph).executionOrder();
}
