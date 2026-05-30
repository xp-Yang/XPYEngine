#ifndef MeshForwardLightingPass_hpp
#define MeshForwardLightingPass_hpp

#include "RenderPass.hpp"

class MeshForwardLightingPass : public RenderPass {
public:
    MeshForwardLightingPass();
    void enableReflection(bool reflection);
    void enablePBR(bool pbr);
    void enableIBL(bool enable) { m_ibl = enable; }
    void enableDirectionalShadow(bool enable) { m_directional_shadow = enable; }
    void draw(RenderPassContext& context) override;

private:
    //params
    bool m_reflection = false;
    bool m_pbr = false;
    bool m_ibl = true;
    bool m_directional_shadow = true;
};

#endif
