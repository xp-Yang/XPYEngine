#ifndef UIPass_hpp
#define UIPass_hpp

#include "RenderPass.hpp"

class UIPass : public RenderPass
{
public:
    UIPass();
    void draw(RenderPassContext& context) override;
};

#endif
