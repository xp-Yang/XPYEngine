#ifndef MeshForwardLightingPass_hpp
#define MeshForwardLightingPass_hpp

#include "RenderPass.hpp"

class MeshForwardLightingPass : public RenderPass {
public:
    MeshForwardLightingPass();
    void enableReflection(bool reflection);
    void enablePBR(bool pbr);
    void draw(RenderPassContext& context) override;

private:
    //params
    bool m_reflection = false;
    bool m_pbr = false;
};

#endif
