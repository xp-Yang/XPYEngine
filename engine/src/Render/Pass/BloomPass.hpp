#ifndef BloomPass_hpp
#define BloomPass_hpp

#include "RenderPass.hpp"
#include <memory>
#include <vector>

class RenderPassContext;

// Mip-chain bloom: progressive downsample -> upsample -> composite.
// Owns its own mip chain textures and FBOs internally, independent of RenderGraph targets.
class BloomPass : public RenderPass
{
public:
    static constexpr int MAX_MIP_LEVELS = 6;
    static constexpr int MIN_MIP_LEVELS = 2;

    struct BloomParams {
        float threshold = 1.0f;
        float softKnee = 0.5f;
        float intensity = 1.0f;
        int   mipLevels = 5;
    };

    BloomPass();
    ~BloomPass() override;

    void setParams(const BloomParams& params) { m_params = params; }
    void draw(RenderPassContext& context) override;

protected:
    void ensureMipChain(const Vec2& scene_size);
    void destroyMipChain();

    void downsample(RenderPassContext& context);
    void upsample(RenderPassContext& context);
    void composite(RenderPassContext& context);

private:
    struct MipLevel {
        Vec2 size;
        RhiTexture* texture{ nullptr };
        std::unique_ptr<RhiFrameBuffer> framebuffer;
    };

    BloomParams m_params;
    Vec2 m_current_scene_size{ 0.f, 0.f };
    int m_current_mip_count{ 0 };
    std::vector<MipLevel> m_mip_chain;

    // Temp FBO for composite to avoid SceneColor read-write feedback loop
    RhiTexture* m_composite_texture{ nullptr };
    std::unique_ptr<RhiFrameBuffer> m_composite_framebuffer;
};

#endif
