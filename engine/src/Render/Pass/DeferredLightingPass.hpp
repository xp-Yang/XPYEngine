#ifndef DeferredLightingPass_hpp
#define DeferredLightingPass_hpp

#include "RenderPass.hpp"

class DeferredLightingPass : public RenderPass {
public:
    DeferredLightingPass();
    void draw(RenderPassContext& context) override;
    void enablePBR(bool enable);
    void enableIBL(bool enable) { m_ibl = enable; }

private:
    bool m_pbr = false;
    bool m_ibl = true;
};

#endif
