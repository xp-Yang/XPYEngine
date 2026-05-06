#ifndef PickingPass_hpp
#define PickingPass_hpp

#include "RenderPass.hpp"

class PickingPass : public RenderPass {
public:
    PickingPass();
    void draw() override;
    void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
    void init() override;

private:
    void rebuildFramebuffer(const Vec2& pixel_size);
};

#endif
