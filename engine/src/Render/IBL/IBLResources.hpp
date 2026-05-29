#ifndef IBLResources_hpp
#define IBLResources_hpp

#include "Render/RHI/rhi.hpp"

// Split-Sum IBL 预计算结果。生命周期由 RenderSystem 持有，运行时由
// Deferred/Forward 光照 pass 绑定到固定纹理单元。
struct IBLResources {
    RhiTexture* environment_cube{ nullptr }; // 环境 cubemap（HDR 时自建；LDR 时借用 skybox）
    RhiTexture* irradiance_cube{ nullptr };  // 漫反射辐照度 32^2
    RhiTexture* prefilter_cube{ nullptr };   // 镜面预滤波 128^2 + mips
    RhiTexture* brdf_lut{ nullptr };         // BRDF 积分 LUT 512^2 (RG)

    bool owns_environment{ false };
    bool ready{ false };

    bool isReady() const
    {
        return ready && irradiance_cube && prefilter_cube && brdf_lut;
    }

    void destroy()
    {
        if (owns_environment)
            delete environment_cube;
        environment_cube = nullptr;
        delete irradiance_cube;
        irradiance_cube = nullptr;
        delete prefilter_cube;
        prefilter_cube = nullptr;
        delete brdf_lut;
        brdf_lut = nullptr;
        ready = false;
        owns_environment = false;
    }
};

#endif // !IBLResources_hpp
