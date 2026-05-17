#ifndef RenderParams_hpp
#define RenderParams_hpp

#include "Base/Common.hpp"

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
    bool bloom = false;
    bool hdr = true;
    bool gamma = true;
    bool fxaa = true;
};

struct EffectParams {
    bool skybox = false;
    bool reflection = false;
    bool wireframe = false;
    bool show_normal = false;
    bool checkerboard = false;
    //int pixelate_level = 1;
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
    MaterialModel material_model = MaterialModel::BlinnPhong;
    MSAAParams msaa_params;
    ShadowParams shadow_params;
    PostProcessingParams post_processing_params;
    EffectParams effect_params;

    RenderResolutionPreset render_resolution = RenderResolutionPreset::FullHD_1080p;

    Vec2 renderTargetPixels() const { return renderResolutionPresetSize(render_resolution); }
};

#endif
