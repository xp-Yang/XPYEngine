#include "CameraManipulator.hpp"

#if ENABLE_ECS
#include "Logical/Framework/ECS/Components.hpp"
#else
#include "Logical/Framework/World/Scene.hpp"
#endif

static void syncCameraObjectTransform(CameraComponent& camera)
{
    if (!camera.parent_object)
        return;

    auto* transform = camera.parent_object->getComponent<TransformComponent>();
    if (!transform)
        return;

    // CameraComponent 是渲染状态的来源，TransformComponent 是场景对象的位置表现。
    // 鼠标/键盘操控相机时，先改 CameraComponent，再把位置镜像回 Transform，
    // 这样层级面板里的 Main Camera 不会停留在旧位置。
    transform->translation = camera.pos;
}

CameraManipulator::CameraManipulator(CameraComponent &camera_)
    : camera(camera_)
    , m_goal_fov(camera_.originFov)
{
}

void CameraManipulator::syncContext(const IntRect &view_rect)
{
    m_view_rect = view_rect;
}

void CameraManipulator::onUpdate()
{
    if (!m_need_update)
        return;

    camera.fov = Math::lerp(camera.fov, m_goal_fov, 0.1f);
    float aspect_ratio = m_view_rect.aspectRatio();
    camera.aspectRatio = aspect_ratio;
    camera.refreshProjection();

    if (Math::isApproxZero(camera.fov - m_goal_fov))
        m_need_update = false;
}

void CameraManipulator::onKeyUpdate(int key, float frame_time)
{
    // 每一帧持续时间越长，意味着上一帧的渲染花费了越多时间，所以这一帧的速度应该越大，来平衡渲染所花去的时间
    float frame_speed = CameraMovementSpeed * frame_time;
    auto camera_forward = camera.direction;
    auto camera_right = camera.getRightDirection();
    auto upDirection = camera.upDirection;
    switch (key)
    {
    case 'W':
        camera.pos += camera_forward * frame_speed;
        break;
    case 'A':
        camera.pos -= camera_right * frame_speed;
        break;
    case 'D':
        camera.pos += camera_right * frame_speed;
        break;
    case 'S':
        camera.pos -= camera_forward * frame_speed;
        break;
    case 'Z':
        camera.pos += upDirection * frame_speed;
        break;
    case 'C':
        camera.pos -= upDirection * frame_speed;
        break;
    default:
        break;
    }
    camera.refreshView();
    syncCameraObjectTransform(camera);
}

void CameraManipulator::onMouseUpdate(double delta_x, double delta_y, MouseButton mouse_button)
{
    if (camera.mode == Mode::Orbit)
    {
        if (mouse_button == MouseButton::Left)
        {
            // TODO 框选多选
        }
        if (mouse_button == MouseButton::Right)
        {
            auto rotate_Y = Math::Rotate(-(float)(0.3f * delta_x * RatationSensitivity), CameraComponent::global_up);
            camera.pos = rotate_Y * Vec4(camera.pos, 1.0f);
            camera.direction = rotate_Y * Vec4(camera.direction, 1.0f);
            camera.direction = Math::Normalize(camera.direction);
            camera.upDirection = Vec3(rotate_Y * Vec4(camera.upDirection, 1.0f));

            auto camera_right = camera.getRightDirection();
            auto rotate_x = Math::Rotate((float)(0.3f * delta_y * RatationSensitivity), camera_right);
            camera.pos = rotate_x * Vec4(camera.pos, 1.0f);
            camera.direction = rotate_x * Vec4(camera.direction, 1.0f);
            camera.upDirection = Vec3(rotate_x * Vec4(camera.upDirection, 1.0f));

            camera.refreshView();
            syncCameraObjectTransform(camera);
        }
        else if (mouse_button == MouseButton::Middle)
        {
            float coef = tan(camera.fov / 2.0f) / tan(camera.originFov / 2.0f);
            camera.pos += -(float)(delta_x * PanSensitivity) * coef * camera.getRightDirection();
            camera.pos += -(float)(delta_y * PanSensitivity) * coef * camera.upDirection;

            camera.refreshView();
            syncCameraObjectTransform(camera);
        }
    }

    if (camera.mode == Mode::FPS)
    {
        if (mouse_button == MouseButton::Left)
        {
            // get pitch
            camera.fps_params.pitch += delta_y * RatationSensitivity;
            // get yaw
            camera.fps_params.yaw += delta_x * RatationSensitivity;

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            // Euler angle problem:
            if (camera.fps_params.pitch > 89.0f)
                camera.fps_params.pitch = 89.0f;
            if (camera.fps_params.pitch < -89.0f)
                camera.fps_params.pitch = -89.0f;

            // update direction
            camera.direction.x = cos(Math::deg2rad(camera.fps_params.pitch)) * sin(Math::deg2rad(camera.fps_params.yaw));
            camera.direction.y = sin(Math::deg2rad(camera.fps_params.pitch));
            camera.direction.z = -cos(Math::deg2rad(camera.fps_params.pitch)) * cos(Math::deg2rad(camera.fps_params.yaw));

            camera.upDirection.x = sin(Math::deg2rad(camera.fps_params.pitch)) * sin(Math::deg2rad(camera.fps_params.yaw));
            camera.upDirection.y = cos(Math::deg2rad(camera.fps_params.pitch));
            camera.upDirection.z = -sin(Math::deg2rad(camera.fps_params.pitch)) * cos(Math::deg2rad(camera.fps_params.yaw));

            camera.refreshView();
            syncCameraObjectTransform(camera);
        }
    }
}

void CameraManipulator::orbitRotate(Vec3 start, Vec3 end)
{
    // 计算旋转角度
    float angle = acos(fmin(1.0f, Math::Dot(start, end)));
    // 计算旋转轴
    Vec3 rotate_axis = Math::Normalize(Math::Cross(start, end));
    // Vec3 world_rotate_axis = Inverse(Mat3(camera.view)) * rotate_axis;

    Mat4 rotate_mat = Math::Rotate(angle, rotate_axis);

    camera.pos = rotate_mat * Vec4(camera.pos, 1.0f);
    camera.direction = rotate_mat * Vec4(camera.direction, 1.0f);
    camera.upDirection = Vec3(rotate_mat * Vec4(camera.upDirection, 1.0f));
    camera.refreshView();
    syncCameraObjectTransform(camera);
}

void CameraManipulator::onMouseWheelUpdate(double yoffset, double mouse_x, double mouse_y)
{
    if (camera.zoom_mode == ZoomMode::ZoomToCenter)
    {
        m_goal_fov = 2 * atan(tan(m_goal_fov / 2.f) / (1 + ZoomUnit * (float)yoffset));
        if (m_goal_fov <= Math::deg2rad(0.01f))
            m_goal_fov = Math::deg2rad(0.01f);
        if (m_goal_fov >= Math::deg2rad(135.0f))
            m_goal_fov = Math::deg2rad(135.0f);

        m_need_update = true;
    }
    if (camera.zoom_mode == ZoomMode::ZoomToMouse)
    {
        Vec3 mouse_3d_pos = rayCastPlaneZero(mouse_x, mouse_y);

        // Logger::debug("Mouse Ray");
        // Logger::debug("Mouse 2d position: {},{}", mouse_x, mouse_y);
        // Logger::debug("Mouse 3d position: {},{},{}", mouse_3d_pos.x, mouse_3d_pos.y, mouse_3d_pos.z);
        // Logger::debug("\n");

        float viewport_width = (float)m_view_rect.width;
        float viewport_height = (float)m_view_rect.height;
        Vec3 center_3d_pos = rayCastPlaneZero(viewport_width / 2.0f, viewport_height / 2.0f);
        Vec3 displacement = mouse_3d_pos - center_3d_pos;

        if (yoffset == 0.0)
            return;
        //// 1. first translate to mouse_3d_pos
        // camera.pos += displacement;
        // float old_zoom = camera.zoom;

        //// 2. set zoom
        // camera.zoom += ZoomUnit * (float)yoffset;
        // if (camera.zoom < 0.1f)
        //     camera.zoom = 0.1f;

        // camera.fov = camera.originFov / camera.zoom;
        // if (camera.fov <= Math::deg2rad(0.01f))
        //     camera.fov = Math::deg2rad(0.01f);
        // if (camera.fov >= Math::deg2rad(135.0f))
        //     camera.fov = Math::deg2rad(135.0f);

        // 3. second translate back to original pos
        // camera.pos -= displacement * (old_zoom / camera.zoom);

        // 4. set view matrix, projection matrix
        camera.refreshView();
        float aspect_ratio = m_view_rect.aspectRatio();
        camera.aspectRatio = aspect_ratio;
        camera.refreshProjection();
    }
}

Vec3 CameraManipulator::rayCastPlaneZero(double mouse_x, double mouse_y)
{

    // 1.  get ray direction from mouse position
    Vec3 cam_right = camera.getRightDirection();
    float viewport_width = (float)m_view_rect.width;
    float viewport_height = (float)m_view_rect.height;
    // normalized the x, y coordinate and take the viewport center as origin
    float u = 2.0f * mouse_x / viewport_width - 1.0f;
    float v = 2.0f * mouse_y / viewport_height - 1.0f;
    v = -v;

    float tangent = std::tan(camera.fov / 2.0f);
    Vec3 ray_direction = camera.direction + cam_right * tangent * u * (m_view_rect.aspectRatio()) + camera.upDirection * tangent * v;
    ray_direction = Math::Normalize(ray_direction);
    // 2.  solve the intersection equation of the ray and the plane:
    // plane_normal. Dot(m_position + t * ray_direction - p0) = 0
    //`Vec3 plane_normal = Vec3(0, 1, 0);
    Vec3 plane_normal = -camera.direction;
    Vec4 zero_plane = Vec4(plane_normal.x, plane_normal.y, plane_normal.z, 0);
    Vec3 p0 = plane_normal * zero_plane[3];
    float t = (Math::Dot(plane_normal, p0) - Math::Dot(plane_normal, camera.pos) / Math::Dot(plane_normal, ray_direction));
    return Vec3(camera.pos + t * ray_direction);
}
