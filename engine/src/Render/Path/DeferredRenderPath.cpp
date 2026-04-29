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

void DeferredRenderPath::render()
{
    const auto& render_params = ref_render_system->renderParams();

    //m_z_pre_pass->draw();

    m_render_passes[RenderPass::Type::Picking]->draw();

    std::vector<RenderPass*> combine_input_passes;
    RenderPass* main_light_pass = nullptr;

    if (render_params.shadow_params.enable) {
        m_render_passes[RenderPass::Type::Shadow]->draw();
    }
    else
        m_render_passes[RenderPass::Type::Shadow]->clear();

    auto gbuffer_pass = static_cast<GBufferPass*>(m_render_passes[RenderPass::Type::GBuffer].get());
    gbuffer_pass->enablePBR(render_params.material_model == MaterialModel::PBR);
    gbuffer_pass->draw();

    auto lighting_pass = static_cast<DeferredLightingPass*>(m_render_passes[RenderPass::Type::DeferredLighting].get());
    lighting_pass->enablePBR(render_params.material_model == MaterialModel::PBR);
    lighting_pass->setCubeMaps(static_cast<ShadowPass*>(m_render_passes[RenderPass::Type::Shadow].get())->getCubeMaps());
    lighting_pass->setInputPasses({ m_render_passes[RenderPass::Type::GBuffer].get(), m_render_passes[RenderPass::Type::Shadow].get() });
    lighting_pass->draw();

    if (render_params.effect_params.skybox) {
        auto skybox_pass = static_cast<SkyBoxPass*>(m_render_passes[RenderPass::Type::SkyBox].get());
        skybox_pass->setInputPasses({ m_render_passes[RenderPass::Type::DeferredLighting].get() }); // draw above the lighting pass framebuffer
        skybox_pass->draw();
    }

    auto transparent_pass = static_cast<TransparentPass*>(m_render_passes[RenderPass::Type::Transparent].get());
    transparent_pass->setInputPasses({ m_render_passes[RenderPass::Type::DeferredLighting].get() }); // draw above the lighting pass framebuffer
    transparent_pass->draw();

    if (render_params.post_processing_params.bloom) {
        m_render_passes[RenderPass::Type::Bloom]->setInputPasses({ m_render_passes[RenderPass::Type::DeferredLighting].get() }); // need extract bright
        m_render_passes[RenderPass::Type::Bloom]->draw();
    }
    else {
        m_render_passes[RenderPass::Type::Bloom]->clear();
    }

    combine_input_passes = { m_render_passes[RenderPass::Type::DeferredLighting].get(), m_render_passes[RenderPass::Type::Bloom].get() };
    main_light_pass = m_render_passes[RenderPass::Type::DeferredLighting].get();


    if (render_params.effect_params.checkerboard) {
        m_render_passes[RenderPass::Type::CheckerBoard]->draw();
        combine_input_passes = { m_render_passes[RenderPass::Type::CheckerBoard].get() };
        main_light_pass = m_render_passes[RenderPass::Type::CheckerBoard].get();
    }
    if (render_params.effect_params.show_normal) {
        m_render_passes[RenderPass::Type::Normal]->setInputPasses({ main_light_pass }); // draw above the main light pass framebuffer
        m_render_passes[RenderPass::Type::Normal]->draw();
    }
    if (render_params.effect_params.wireframe) {
        m_render_passes[RenderPass::Type::WireFrame]->setInputPasses({ main_light_pass }); // draw above the main light pass framebuffer
        m_render_passes[RenderPass::Type::WireFrame]->draw();
    }

    m_render_passes[RenderPass::Type::Outline]->setInputPasses({ main_light_pass }); // draw above the main light pass framebuffer
    m_render_passes[RenderPass::Type::Outline]->draw();

    auto combine_pass = static_cast<CombinePass*>(m_render_passes[RenderPass::Type::Combined].get());
    combine_pass->setInputPasses(combine_input_passes);
    combine_pass->enableFXAA(render_params.post_processing_params.fxaa);
    combine_pass->draw();
}
