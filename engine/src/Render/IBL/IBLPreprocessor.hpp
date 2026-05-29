#ifndef IBLPreprocessor_hpp
#define IBLPreprocessor_hpp

#include "IBLResources.hpp"

#include <string>

// 一次性 IBL 预计算器。采用与引擎纹理创建一致的“裸 OpenGL”实现：
// 在启动阶段把环境贴图转换为 irradiance / prefilter / BRDF LUT。
// 输出纹理通过 RHI 创建（保持 RhiTexture* 句柄，便于运行时绑定），
// 但渲染进这些纹理的过程使用自管理的 FBO 与内联着色器，避免侵入
// 渲染图 / 管线库。
class IBLPreprocessor {
public:
    // 从等距柱状 HDR (.hdr) 构建完整 IBL（自建 RGB16F 环境 cubemap）。
    bool buildFromEquirectHDR(const std::string& hdr_path, IBLResources& out);

    // 从已有环境 cubemap（如 skybox 的 6 面 JPG cube）派生 IBL，
    // environment_cube 直接借用传入句柄（不接管其生命周期）。
    bool buildFromEnvironmentCube(RhiTexture* environment_cube, IBLResources& out);

private:
    // environment_cube 就绪后，派生 irradiance / prefilter / brdfLUT。
    bool deriveFromEnvironment(IBLResources& out);
};

#endif // !IBLPreprocessor_hpp
