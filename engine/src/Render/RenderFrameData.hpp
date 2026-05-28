#ifndef RenderFrameData_hpp
#define RenderFrameData_hpp

#include "Base/Common.hpp"
#include "Logical/Framework/Object/GObject.hpp"

#include <array>
#include <memory>
#include <vector>

static const size_t MAX_CUBE_SHADOW_MAP_COUNT = 8;

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
    std::array<Mat4, 6> lightViewMatrix;
    Mat4 lightProjMatrix;
};

struct RenderCameraData {
    float fov;
    Vec3 pos;
    Vec3 direction;
    Vec3 rightDirection;
    Vec3 upDirection;
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

    std::shared_ptr<RenderCameraData> render_camera;

    int point_light_inst_amount{ 0 };
};

#endif // !RenderFrameData_hpp
