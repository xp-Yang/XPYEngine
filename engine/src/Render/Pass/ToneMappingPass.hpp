#ifndef ToneMappingPass_hpp
#define ToneMappingPass_hpp

#include "RenderPass.hpp"
#include <memory>

class ToneMappingPass : public RenderPass
{
public:
    ToneMappingPass();
    ~ToneMappingPass() override;

    void setExposure(float exposure) { m_exposure = exposure; }
    void draw(RenderPassContext& context) override;

private:
    void ensureTempBuffer(const Vec2& scene_size);
    void destroyTempBuffer();

    float m_exposure = 1.0f;
    Vec2 m_current_scene_size{ 0.f, 0.f };
    RhiTexture* m_temp_texture{ nullptr };
    std::unique_ptr<RhiFrameBuffer> m_temp_framebuffer;
};

#endif
