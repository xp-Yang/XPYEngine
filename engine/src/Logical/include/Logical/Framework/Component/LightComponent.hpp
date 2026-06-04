#ifndef LightComponent_hpp
#define LightComponent_hpp

#include "Logical/Framework/Component/Component.hpp"

#include <array>

struct LightComponent : public Component {
    LightComponent(GObject* parent) : Component(parent) {}

    Color3 luminousColor{ 1.0f, 1.0f, 1.0f };
};

struct PointLightComponent : public LightComponent {
    PointLightComponent(GObject* parent) : LightComponent(parent) {}

    float radius{ 30.0f };
    bool castShadow{ true };

    Mat4 lightProjMatrix() const
    {
        return Math::Perspective(Math::deg2rad(90.0f), 1.0f, 0.1f, radius);
    }

    std::array<Mat4, 6> lightViewMatrix(const Vec3& position) const
    {
        // 原 LightManager.hpp 里的 TODO：
        // “这里up向量向下，因为cubeMap从内部采样，是反过来的”
        // “点阴影贴图 up 向量朝下，那么上下不也颠倒了吗？”
        //
        // 答案：不会在最终采样意义上颠倒。点光阴影写入的是 cubemap，
        // 每个 face 都有 OpenGL/图形 API 约定的局部坐标方向。对 +X/-X/+Z/-Z
        // 这些 face 使用 (0, -1, 0) 作为 up，并不是把世界整体倒过来，而是为了让
        // light-space view 矩阵和 cubemap face 的采样方向保持一致，避免 face 之间
        // 出现旋转错位或接缝。
        //
        // 单独把某一张 face 当 2D 图预览时，它可能看起来是“倒的”；但 shader 用
        // fragPos - lightPos 这样的 3D 方向向量采样 cubemap 时，采样和写入都遵守
        // 同一套 face 方向约定，所以世界里的上下关系不会因此错乱。真正需要小心的是：
        // 下面 6 个矩阵的顺序必须和创建/绑定 cubemap face 的顺序完全一致。
        std::array<Mat4, 6> result;
        result[0] = Math::LookAt(position, position + Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f));
        result[1] = Math::LookAt(position, position + Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f));
        result[2] = Math::LookAt(position, position + Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
        result[3] = Math::LookAt(position, position + Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));
        result[4] = Math::LookAt(position, position + Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f));
        result[5] = Math::LookAt(position, position + Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, -1.0f, 0.0f));
        return result;
    }
};

struct DirectionalLightComponent : public LightComponent {
    DirectionalLightComponent(GObject* parent) : LightComponent(parent) {}

    Vec3 direction{ 15.0f, -30.0f, 15.0f };
    float aspectRatio{ 16.0f / 9.0f };

    Mat4 lightProjMatrix() const
    {
        // 原 LightManager.hpp 里的 TODO：
        // “方向光阴影覆盖过窄/过宽、边缘裁剪、分辨率利用率变差，怎么解决？”
        //
        // 答案：当前固定正交投影盒只是一个简单默认值。它的缺点很明确：
        // 盒子太小会裁掉阴影，盒子太大又会把 shadow map 分辨率浪费在不可见区域，
        // 导致阴影变糊。
        //
        // 更合理的方案是动态拟合 light frustum：
        // 1. 取主相机视锥在 shadowDistance 内的 8 个角点。
        // 2. 把这些角点变换到 light view space。
        // 3. 用角点的 min/max 生成紧包围的 ortho left/right/bottom/top/near/far。
        // 4. 把 ortho 中心按 shadow map texel size 对齐，减少相机移动时的阴影抖动。
        //
        // 大场景还应该进一步做 CSM，也就是把相机视锥按深度切成多个 cascade，
        // 每个 cascade 单独拟合一个正交投影。这样近处阴影清晰，远处阴影覆盖范围足够。
        // 目前这里保留固定盒子，适合作为简单场景的 fallback。
        return Math::Ortho(-30.0f * aspectRatio, 30.0f * aspectRatio, -30.0f, 30.0f, 0.1f, 100.0f);
    }

    Mat4 lightViewMatrix() const
    {
        return Math::LookAt(Vec3(0.0f) - direction, Vec3(0.0f), Vec3(0.0f, 1.0f, 0.0f));
    }
};

#endif // !LightComponent_hpp
