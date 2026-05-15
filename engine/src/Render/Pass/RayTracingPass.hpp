#ifndef RayTracingPass_hpp
#define RayTracingPass_hpp

#include "RenderPass.hpp"

class RayTracingPass : public RenderPass {
public:
    RayTracingPass();
    void draw(RenderPassContext& context) override;
};

#endif
