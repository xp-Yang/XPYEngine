#ifndef FinalPass_hpp
#define FinalPass_hpp

#include "RenderPass.hpp"

class FinalPass : public RenderPass
{
public:
    FinalPass();
    void draw(RenderPassContext& context) override;
};

#endif
