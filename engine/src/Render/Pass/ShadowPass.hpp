#ifndef ShadowPass_hpp 
#define ShadowPass_hpp

#include "RenderPass.hpp"

class ShadowPass : public RenderPass {
public:
    ShadowPass();
    void draw() override;
    void clear() override;
    const std::vector<unsigned int>& getCubeMaps() const { return m_cube_maps; }
    void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
    void init() override;
    void drawDirectionalLightShadowMap();
    void drawPointLightShadowMap();

    void reinit_cube_maps(size_t count);

private:
    void rebuildFramebuffer(const Vec2& pixel_size, size_t cube_map_count = 8);

    std::vector<unsigned int> m_cube_maps;
    unsigned int m_cube_map_fbo = 0;
    int m_point_shadow_cube_edge{ 1080 };
};

#endif
