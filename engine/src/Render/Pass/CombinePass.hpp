#ifndef CombinePass_hpp
#define CombinePass_hpp

#include "RenderPass.hpp"

class CombinePass : public RenderPass
{
public:
    CombinePass();
    void draw(RenderPassContext& context) override;
    void enableFXAA(bool enable);

private:
    bool m_fxaa = false;
};

#endif
