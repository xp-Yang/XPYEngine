#ifndef ToneMappingPass_hpp
#define ToneMappingPass_hpp

#include "RenderPass.hpp"

class ToneMappingPass : public RenderPass
{
public:
    ToneMappingPass();

    void setExposure(float exposure) { m_exposure = exposure; }
    void draw(RenderPassContext& context) override;

private:
    float m_exposure = 1.0f;
};

#endif
