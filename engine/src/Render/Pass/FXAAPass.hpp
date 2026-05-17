#ifndef FXAAPass_hpp
#define FXAAPass_hpp

#include "RenderPass.hpp"

class FXAAPass : public RenderPass
{
public:
    FXAAPass();
    void draw(RenderPassContext& context) override;
};

#endif
