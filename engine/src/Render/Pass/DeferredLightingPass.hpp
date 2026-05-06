#ifndef DeferredLightingPass_hpp
#define DeferredLightingPass_hpp

#include "RenderPass.hpp"

class DeferredLightingPass : public RenderPass {
public:
    DeferredLightingPass();
    void draw() override;
    void enablePBR(bool enable);
    void setCubeMaps(const std::vector<unsigned int>& cube_maps) { m_cube_maps = cube_maps; }
    void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
    void init() override;

private:
    void rebuildFramebuffer(const Vec2& pixel_size);
    bool m_pbr = false;

    unsigned int m_dir_light_shadow_map = 0;
    std::vector<unsigned int> m_cube_maps;
};

#endif
