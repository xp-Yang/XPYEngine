#include "ForwardRenderPath.hpp"

#include "../Pass/ShadowPass.hpp"
#include "../Pass/MeshForwardLightingPass.hpp"
#include "../Pass/PickingPass.hpp"
#include "../Pass/SkyBoxPass.hpp"
#include "../Pass/OutlinePass.hpp"
#include "../Pass/CombinePass.hpp"

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

void ForwardRenderPath::render()
{
    auto render_params = ref_render_system->renderParams();

    // TODO samples更改了，重新初始化整个path，重新创建相关的RenderPass。
    // configShadowMapSamples(render_params.shadow_map_sample_count);
    // configSamples(render_params.msaa_sample_count);
    // if (render_params.shadow) {
    //     m_shadow_pass->draw();
    // }
    // else {
    //     static_cast<ShadowPass*>(m_shadow_pass.get())->clear();
    // }

    m_render_passes[RenderPass::Type::Picking]->draw();
    m_render_passes[RenderPass::Type::Shadow]->draw();
    MeshForwardLightingPass *main_pass = static_cast<MeshForwardLightingPass *>(m_render_passes[RenderPass::Type::Forward].get());
    main_pass->setInputPasses({m_render_passes[RenderPass::Type::Shadow].get()});
    main_pass->enablePBR(render_params.material_model == MaterialModel::PBR);
    main_pass->enableReflection(render_params.effect_params.reflection);
    main_pass->setCubeMaps(static_cast<ShadowPass *>(m_render_passes[RenderPass::Type::Shadow].get())->getCubeMaps());
    main_pass->draw();
    if (render_params.effect_params.skybox)
    {
        auto skybox_pass = static_cast<SkyBoxPass *>(m_render_passes[RenderPass::Type::SkyBox].get());
        skybox_pass->setInputPasses({main_pass}); // draw above the lighting pass framebuffer
        skybox_pass->draw();
    }
    m_render_passes[RenderPass::Type::Outline]->setInputPasses({main_pass}); // draw above the main light pass framebuffer
    m_render_passes[RenderPass::Type::Outline]->draw();
    m_render_passes[RenderPass::Type::Combined]->setInputPasses({main_pass});

    static_cast<CombinePass *>(m_render_passes[RenderPass::Type::Combined].get())->enableFXAA(render_params.post_processing_params.fxaa);
    m_render_passes[RenderPass::Type::Combined]->draw();
}