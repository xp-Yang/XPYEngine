#ifndef GBufferPass_hpp
#define GBufferPass_hpp

#include "RenderPass.hpp"

class GBufferPass : public RenderPass {
public:
    GBufferPass();
    void draw() override;
    void enablePBR(bool enable);
    void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
    void init() override;

private:
    void rebuildFramebuffer(const Vec2& pixel_size);
    bool m_pbr = false;
};

#endif // !GBufferPass_hpp