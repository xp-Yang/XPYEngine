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

void DeferredRenderPath::render(RenderSourceData& render_source_data)
{
    const auto& render_params = ref_render_system->renderParams();
    const bool checkerboard_enabled = render_params.effect_params.checkerboard;
    const bool bloom_used = render_params.post_processing_params.bloom && !checkerboard_enabled;
    auto pass = [this](RenderPass::Type type) -> RenderPass*
    {
        return m_render_passes.at(type).get();
    };

    m_render_graph.reset();

    m_render_graph.addPass(RenderPass::Type::Picking, pass(RenderPass::Type::Picking))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .color(RGResource::PickingColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::PickingDepth, RhiTexture::Format::DEPTH);

    m_render_graph.addPass(RenderPass::Type::Shadow, pass(RenderPass::Type::Shadow))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .setEnabled(render_params.shadow_params.enable)
        .setDisabledExecution(RGDisabledExecution::Clear)
        .color(RGResource::ShadowDirectionalColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::ShadowDirectionalDepth, RhiTexture::Format::DEPTH)
        .target(RGTarget::ShadowPointDepth, RenderTargetType::CubeDepth, 8);

    auto& gbuffer_node = m_render_graph.addPass(RenderPass::Type::GBuffer, pass(RenderPass::Type::GBuffer))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
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

    auto& lighting_node = m_render_graph.addPass(RenderPass::Type::DeferredLighting, pass(RenderPass::Type::DeferredLighting))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .read(RGResource::GBufferPosition)
        .read(RGResource::GBufferNormal)
        .read(RGResource::ShadowDirectionalDepth)
        .color(RGResource::SceneColor, RhiTexture::Format::RGBA16F)
        .depth(RGResource::SceneDepth, RhiTexture::Format::DEPTH)
        .setSetup([this, &render_params](RenderPass& render_pass)
        {
            auto& lighting_pass = static_cast<DeferredLightingPass&>(render_pass);
            lighting_pass.enablePBR(render_params.material_model == MaterialModel::PBR);
            lighting_pass.setCubeMaps(m_render_graph.cubeDepthTextures(RenderPass::Type::Shadow));
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

    m_render_graph.addPass(RenderPass::Type::SkyBox, pass(RenderPass::Type::SkyBox))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .setEnabled(render_params.effect_params.skybox)
        .readWrite(RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth);

    m_render_graph.addPass(RenderPass::Type::Transparent, pass(RenderPass::Type::Transparent))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .readWrite(RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth);

    m_render_graph.addPass(RenderPass::Type::Bloom, pass(RenderPass::Type::Bloom))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .setEnabled(bloom_used)
        .setDisabledExecution(RGDisabledExecution::Clear)
        .read(RGResource::SceneColor)
        .color(RGResource::BloomColor, RhiTexture::Format::RGB16F)
        .target(RGTarget::BloomPingPong, RenderTargetType::FrameBuffer)
        .color(RGResource::BloomPingPongColor, RhiTexture::Format::RGB16F);

    m_render_graph.addPass(RenderPass::Type::CheckerBoard, pass(RenderPass::Type::CheckerBoard))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .setEnabled(checkerboard_enabled)
        .color(RGResource::SceneColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::SceneDepth, RhiTexture::Format::DEPTH);

    m_render_graph.addPass(RenderPass::Type::Normal, pass(RenderPass::Type::Normal))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .setEnabled(render_params.effect_params.show_normal)
        .readWrite(RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth);

    m_render_graph.addPass(RenderPass::Type::WireFrame, pass(RenderPass::Type::WireFrame))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .setEnabled(render_params.effect_params.wireframe)
        .readWrite(RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth);

    m_render_graph.addPass(RenderPass::Type::Outline, pass(RenderPass::Type::Outline))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .readWrite(RGResource::SceneColor)
        .readWrite(RGResource::SceneDepth)
        .target(RGTarget::OutlineMask, RenderTargetType::FrameBuffer)
        .color(RGResource::OutlineMaskColor, RhiTexture::Format::RGB16F)
        .depth(RGResource::OutlineMaskDepth, RhiTexture::Format::DEPTH);

    auto& combine_node = m_render_graph.addPass(RenderPass::Type::Combined, pass(RenderPass::Type::Combined))
        .target(RGTarget::Main, RenderTargetType::FrameBuffer)
        .read(RGResource::SceneColor)
        .read(RGResource::SceneDepth)
        .color(RGResource::FinalColor, RhiTexture::Format::RGB8)
        .depth(RGResource::FinalDepth, RhiTexture::Format::DEPTH)
        .target(RGTarget::Backbuffer, RenderTargetType::Backbuffer)
        .setSetup([&render_params](RenderPass& render_pass)
        {
            static_cast<CombinePass&>(render_pass).enableFXAA(render_params.post_processing_params.fxaa);
        });

    if (bloom_used)
        combine_node.read(RGResource::BloomColor);

    m_render_graph.markOutput(RGResource::PickingColor);
    if (!bloom_used)
        m_render_graph.markOutput(RGResource::BloomColor);
    m_render_graph.markOutput(RGResource::FinalColor);

    m_render_graph.compile();
    m_render_graph.execute(render_source_data);
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

std::string DeferredRenderPath::renderGraphDebugDump() const
{
    return RenderGraphDumper(m_render_graph).graph();
}

std::string DeferredRenderPath::renderGraphExecutionDump() const
{
    return RenderGraphDumper(m_render_graph).executionOrder();
}
