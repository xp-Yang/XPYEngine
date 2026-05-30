#ifndef ShadowPass_hpp 
#define ShadowPass_hpp

#include "RenderPass.hpp"

class ShadowPass : public RenderPass {
public:
    ShadowPass();
    void draw(RenderPassContext& context) override;

    void setRenderDirectionalShadows(bool enable) { m_render_directional_shadows = enable; }
    void setRenderPointShadows(bool enable) { m_render_point_shadows = enable; }

protected:
    void drawDirectionalLightShadowMap(RenderPassContext& context);
    void drawPointLightShadowMap(RenderPassContext& context);

private:
    bool m_render_directional_shadows{ true };
    bool m_render_point_shadows{ true };
};

#endif
