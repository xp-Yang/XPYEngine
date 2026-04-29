#ifndef Material_hpp
#define Material_hpp

#include "Base/Common.hpp"
#include "Texture.hpp"

struct Material {
    // TODO
    //CullingMode
    //TransparencyMode
    //DepthFunc
    //StencilCompareFunc
    //StencilOperation
    //StencilFace
    static std::shared_ptr<Material> create_complete_default_material();

    Material() = default;

    // pbr
    std::shared_ptr<Texture> albedo_texture;
    std::shared_ptr<Texture> metallic_texture;
    std::shared_ptr<Texture> roughness_texture;
    std::shared_ptr<Texture> ao_texture;

    //// temp
    //Vec3 albedo{ Vec3(1.0f) };
    //float metallic{ 1.0f };
    //float roughness{ 0.5f };
    //float ao{ 0.01f };

    // blinn phong
    std::shared_ptr<Texture> diffuse_texture;
    std::shared_ptr<Texture> specular_texture;
    std::shared_ptr<Texture> normal_texture;
    std::shared_ptr<Texture> height_texture;

    float alpha{ 1.0f };
};

#endif