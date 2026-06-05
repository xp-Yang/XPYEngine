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
        // “点阴影贴图 up 向量朝下，那么上下不是颠倒了吗？”
        // 答案：https://wikis.khronos.org/opengl/Cubemap_Texture
        // 立方体纹理的坐标系是左旋的。对 +X/-X/+Z/-Z 这些 face 使用 (0, -1, 0) 作为 up
        std::array<Mat4, 6> result;
        result[0] = Math::LookAt(position, position + Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)); // +X
        result[1] = Math::LookAt(position, position + Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f));// -X
        result[2] = Math::LookAt(position, position + Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));  // +Y
        result[3] = Math::LookAt(position, position + Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f));// -Y
        result[4] = Math::LookAt(position, position + Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f)); // +Z
        result[5] = Math::LookAt(position, position + Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, -1.0f, 0.0f));// -Z
        return result;
    }
};

struct DirectionalLightComponent : public LightComponent {
    DirectionalLightComponent(GObject* parent) : LightComponent(parent) {}

    Vec3 direction{ 15.0f, -30.0f, 15.0f };
    float aspectRatio{ 16.0f / 9.0f };

    Mat4 lightProjMatrix() const
    {
        // TODO
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
