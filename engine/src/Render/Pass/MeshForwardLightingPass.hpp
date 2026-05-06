#ifndef MeshForwardLightingPass_hpp
#define MeshForwardLightingPass_hpp

#include "RenderPass.hpp"

class MeshForwardLightingPass : public RenderPass {
public:
    MeshForwardLightingPass();
    void enableReflection(bool reflection);
    void enablePBR(bool pbr);
    void draw() override;
    void setCubeMaps(const std::vector<unsigned int>& cube_maps) { m_cube_maps = cube_maps; }
    void rebuildFramebuffers(const Vec2& pixel_size) override;

protected:
    void init() override;

private:
    void rebuildFramebuffer(const Vec2& pixel_size);
    unsigned int m_shadow_map = 0;
    std::vector<unsigned int> m_cube_maps;

    //params
    bool m_reflection = false;
    bool m_pbr = false;
};

#endif
