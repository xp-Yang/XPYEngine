#ifndef RenderParams_hpp
#define RenderParams_hpp

#include "Base/Common.hpp"

#include <string>

enum class RenderPathType {
    Forward,
    Deferred,
};

enum class MaterialModel {
    BlinnPhong,
    PBR,
};

struct MSAAParams {
    int sample_count = 1;
};

struct ShadowParams {
    bool enable = true;
    int sample_count = 1;
};

struct PostProcessingParams {
    bool bloom = true;
    float bloom_threshold = 1.0f;
    float bloom_soft_knee = 0.3f;
    float bloom_intensity = 1.0f;
    int   bloom_mip_levels = 5;

    bool hdr = true;
    bool gamma = true;
    bool fxaa = true;

    bool tone_mapping = true;
    float exposure = 1.0f;
};

struct EffectParams {
    bool skybox = true;
    bool reflection = false;
    bool wireframe = false;
    bool show_normal = false;
    bool checkerboard = false;
    bool grid = true; // FinalPass pristine grid overlay
    //int pixelate_level = 1;
};

struct IBLParams {
    bool enable = true;
    // 可选等距柱状 HDR 路径（相对 ASSET_DIR 或绝对路径）。
    // 为空时回退到从已有 skybox cubemap 派生 IBL。
    std::string env_hdr_path;
};

struct SSAOParams {
    bool  enable = true;
    float radius = 0.5f;
    float bias = 0.025f;
    float power = 1.0f;
};

/** Preset internal render target resolution (offscreen FBOs). Not tied to window size. */
enum class RenderResolutionPreset {
    FullHD_1080p, /**< 1920 x 1080 */
    QHD_1440p,    /**< 2560 x 1440 ("2K") */
    UHD_4K,       /**< 3840 x 2160 */
};

inline Vec2 renderResolutionPresetSize(RenderResolutionPreset preset)
{
    switch (preset)
    {
    case RenderResolutionPreset::FullHD_1080p:
        return Vec2(1920.f, 1080.f);
    case RenderResolutionPreset::QHD_1440p:
        return Vec2(2560.f, 1440.f);
    case RenderResolutionPreset::UHD_4K:
        return Vec2(3840.f, 2160.f);
    default:
        return Vec2(1920.f, 1080.f);
    }
}

struct RenderParams {
    RenderPathType render_path_type = RenderPathType::Deferred;
    MaterialModel material_model = MaterialModel::PBR;
    MSAAParams msaa_params;
    ShadowParams shadow_params;
    PostProcessingParams post_processing_params;
    EffectParams effect_params;
    IBLParams ibl;
    SSAOParams ssao;

    RenderResolutionPreset render_resolution = RenderResolutionPreset::UHD_4K;

    Vec2 renderTargetPixels() const { return renderResolutionPresetSize(render_resolution); }
};

#endif
