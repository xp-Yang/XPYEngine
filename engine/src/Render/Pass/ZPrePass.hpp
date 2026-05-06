#ifndef ZPrePass_hpp
#define ZPrePass_hpp

#include "RenderPass.hpp"

class ZPrePass : public RenderPass {
public:
    ZPrePass();
    void draw() override;
    void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
    void init() override;

private:
    void rebuildFramebuffer(const Vec2& pixel_size);
};

#endif // !ZPrePass_hpp