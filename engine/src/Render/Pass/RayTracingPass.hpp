#ifndef RayTracingPass_hpp
#define RayTracingPass_hpp

#include "RenderPass.hpp"

class RayTracingPass : public RenderPass {
public:
    RayTracingPass();
    void draw() override;
    void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
    void init() override;

private:
    void rebuildFramebuffer(const Vec2& pixel_size);
};

#endif
