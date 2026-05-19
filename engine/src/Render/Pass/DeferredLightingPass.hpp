#ifndef DeferredLightingPass_hpp
#define DeferredLightingPass_hpp

#include "RenderPass.hpp"

class DeferredLightingPass : public RenderPass {
public:
    DeferredLightingPass();
    void draw(RenderPassContext& context) override;
    void enablePBR(bool enable);

private:
    bool m_pbr = false;
};

#endif
