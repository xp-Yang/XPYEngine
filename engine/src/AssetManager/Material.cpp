#include "AssetManager/Material.hpp"

#include <algorithm>
#include <cmath>

static std::shared_ptr<Texture> defaultWhiteTexture(TextureType type)
{
    return std::make_shared<Texture>(type, std::string(ASSET_DIR) + "/images/pure_white_map.png", false);
}

std::shared_ptr<Material> Material::create_complete_default_material()
{
    auto default_material = std::make_shared<Material>();
    default_material->diffuse_texture = defaultWhiteTexture(TextureType::Diffuse);
    default_material->specular_texture = defaultWhiteTexture(TextureType::Specular);
    default_material->normal_texture = defaultWhiteTexture(TextureType::Normal);
    default_material->height_texture = defaultWhiteTexture(TextureType::Height);
    default_material->albedo_texture = defaultWhiteTexture(TextureType::Albedo);
    default_material->metallic_texture = defaultWhiteTexture(TextureType::Metallic);
    default_material->roughness_texture = defaultWhiteTexture(TextureType::Roughness);
    default_material->ao_texture = defaultWhiteTexture(TextureType::AO);
    default_material->metallic_factor = 0.0f;
    default_material->roughness_factor = 1.0f;
    default_material->ao_factor = 1.0f;
    default_material->shininess = 128.0f;
    return default_material;
}

void Material::fillPBRFromBlinnPhong()
{
    auto roughnessFromBlinnPhongShininess = [](float shininess)
    {
        shininess = std::max(shininess, 1.0f);
        return std::clamp(std::sqrt(2.0f / (shininess + 2.0f)), 0.04f, 1.0f);
    };

    this->albedo_texture = this->diffuse_texture ? this->diffuse_texture : defaultWhiteTexture(TextureType::Albedo);
    if (!this->metallic_texture)
        this->metallic_texture = defaultWhiteTexture(TextureType::Metallic);
    if (!this->roughness_texture)
        this->roughness_texture = defaultWhiteTexture(TextureType::Roughness);
    if (!this->ao_texture)
        this->ao_texture = defaultWhiteTexture(TextureType::AO);

    this->base_color_factor = this->diffuse_factor;
    this->metallic_factor = 0.0f;
    this->roughness_factor = roughnessFromBlinnPhongShininess(this->shininess);
    this->ao_factor = 1.0f;
    this->markDirty();
}

void Material::fillBlinnPhongFromPBR()
{
    auto blinnPhongShininessFromRoughness = [](float roughness)
    {
        roughness = std::clamp(roughness, 0.04f, 1.0f);
        return std::clamp(2.0f / (roughness * roughness) - 2.0f, 1.0f, 1024.0f);
    };

    this->diffuse_texture = this->albedo_texture ? this->albedo_texture : defaultWhiteTexture(TextureType::Diffuse);
    if (!this->specular_texture)
        this->specular_texture = defaultWhiteTexture(TextureType::Specular);
    if (!this->normal_texture)
        this->normal_texture = defaultWhiteTexture(TextureType::Normal);
    if (!this->height_texture)
        this->height_texture = defaultWhiteTexture(TextureType::Height);

    const float metallic = std::clamp(this->metallic_factor, 0.0f, 1.0f);
    const float roughness = std::clamp(this->roughness_factor, 0.04f, 1.0f);
    const Vec3 base_color = this->base_color_factor;

    this->diffuse_factor = base_color * (1.0f - metallic);
    this->specular_factor = Math::lerp(Vec3(0.04f), base_color, metallic);
    this->shininess = blinnPhongShininessFromRoughness(roughness);
    this->markDirty();
}
