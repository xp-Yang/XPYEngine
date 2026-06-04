#ifndef RenderFrameData_hpp
#define RenderFrameData_hpp

#include "Base/Common.hpp"
#include "Logical/Framework/Object/GObject.hpp"

#include <array>
#include <memory>
#include <vector>

// 注意：与 GLSL common.h 的 MAX_POINT_LIGHTS_COUNT 保持一致。
// 降为 5 是为了给 IBL 三张纹理让出 sampler 单元（见 IBL 方案“纹理单元预算”）。
static const size_t MAX_CUBE_SHADOW_MAP_COUNT = 5;

struct RenderDirectionalLightData {
    Color3 color;
    Vec3 direction;
    Mat4 lightViewMatrix;
    Mat4 lightProjMatrix;
};

struct RenderPointLightData {
    int id;
    Color3 color;
    Vec3 position;
    float radius;
    int shadow_index{ -1 };
    std::array<Mat4, 6> lightViewMatrix;
    Mat4 lightProjMatrix;
};

// Per-frame transient data built from the active view and the current scene state.
struct RenderFrameData {
    // Reset only data that is reconstructed every frame.
    void reset();

    std::vector<RenderDirectionalLightData> directional_lights;
    std::vector<RenderPointLightData> point_lights;
    std::vector<GObjectID> picked_ids;

    Vec3 camera_position;
    Mat4 view_matrix;
    Mat4 proj_matrix;

    int point_light_inst_amount{ 0 };
};

#endif // !RenderFrameData_hpp
