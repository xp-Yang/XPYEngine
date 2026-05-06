#ifndef Material_hpp
#define Material_hpp

#include "Base/Common.hpp"
#include "Texture.hpp"

#include <cstdint>

struct Material
{
    // TODO
    // CullingMode
    // TransparencyMode
    // DepthFunc
    // StencilCompareFunc
    // StencilOperation
    // StencilFace
    static std::shared_ptr<Material> create_complete_default_material();

    Material() = default;

    void markDirty() { ++version; }

    uint64_t version{0};

    // pbr
    std::shared_ptr<Texture> albedo_texture;
    std::shared_ptr<Texture> metallic_texture;
    std::shared_ptr<Texture> roughness_texture;
    std::shared_ptr<Texture> ao_texture;

    Vec4 base_color_factor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic_factor{1.0f};
    float roughness_factor{1.0f};
    float ao_factor{1.0f};

    // blinn phong
    std::shared_ptr<Texture> diffuse_texture;
    std::shared_ptr<Texture> specular_texture;
    std::shared_ptr<Texture> normal_texture;
    std::shared_ptr<Texture> height_texture;

    Vec3 diffuse_factor{1.0f, 1.0f, 1.0f};
    Vec3 specular_factor{1.0f, 1.0f, 1.0f};

    float alpha{1.0f};
};

#endif
