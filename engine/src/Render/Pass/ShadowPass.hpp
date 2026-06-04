#ifndef ShadowPass_hpp
#define ShadowPass_hpp

#include "RenderPass.hpp"
#include "Render/RenderFrameData.hpp"

#include <array>
#include <cstdint>

class ShadowPass : public RenderPass {
public:
    ShadowPass();
    void draw(RenderPassContext& context) override;

    void setRenderDirectionalShadows(bool enable) { m_render_directional_shadows = enable; }
    void setRenderPointShadows(bool enable) { m_render_point_shadows = enable; }

protected:
    void drawDirectionalLightShadowMap(RenderPassContext& context);
    void drawPointLightShadowMap(RenderPassContext& context);

private:
    struct PointShadowCacheEntry {
        int light_id{ -1 };
        Vec3 position{ 0.0f };
        float radius{ 0.0f };
        uint64_t static_version{ 0 };
        std::array<bool, 6> face_valid{};
        std::array<RhiFrameBuffer*, 6> static_face_framebuffers{};
    };

    bool m_render_directional_shadows{ true };
    bool m_render_point_shadows{ true };
    std::array<PointShadowCacheEntry, MAX_CUBE_SHADOW_MAP_COUNT> m_point_shadow_cache{};
};

#endif
