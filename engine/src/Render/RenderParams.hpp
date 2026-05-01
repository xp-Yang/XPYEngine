#ifndef RenderParams_hpp
#define RenderParams_hpp

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
    bool fxaa = false;
};

struct EffectParams {
    bool skybox = false;
    bool reflection = false;
    bool wireframe = false;
    bool show_normal = false;
    bool checkerboard = false;
    //int pixelate_level = 1;
};

struct RenderParams {
    RenderPathType render_path_type = RenderPathType::Deferred;
    MaterialModel material_model = MaterialModel::BlinnPhong;
    MSAAParams msaa_params;
    ShadowParams shadow_params;
    PostProcessingParams post_processing_params;
    EffectParams effect_params;
};

#endif
