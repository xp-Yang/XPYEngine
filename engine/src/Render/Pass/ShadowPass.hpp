#ifndef ShadowPass_hpp 
#define ShadowPass_hpp

#include "RenderPass.hpp"

class ShadowPass : public RenderPass {
public:
    ShadowPass();
    void draw(RenderPassContext& context) override;

protected:
    void drawDirectionalLightShadowMap(RenderPassContext& context);
    void drawPointLightShadowMap(RenderPassContext& context);

private:
};

#endif
