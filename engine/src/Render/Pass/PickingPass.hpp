#ifndef PickingPass_hpp
#define PickingPass_hpp

#include "RenderPass.hpp"

class PickingPass : public RenderPass {
public:
    PickingPass();
    void draw(RenderPassContext& context) override;
};

#endif
