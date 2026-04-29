#include "CameraManipulator.hpp"

#if ENABLE_ECS
#include "Logical/Framework/ECS/Components.hpp"
#else
#include "Logical/Framework/World/Scene.hpp"
#endif

CameraManipulator::CameraManipulator(CameraComponent &camera)
    : main_camera(camera)
{
    m_goal_fov = main_camera.originFov;
}

void CameraManipulator::syncContext(const Viewport &viewport)
{
    m_viewport = viewport;
}

void CameraManipulator::onUpdate()
{
    if (!m_need_update)
        return;

    main_camera.fov = Math::lerp(main_camera.fov, m_goal_fov, 0.1f);
    float aspect_ratio = m_viewport.AspectRatio();
    main_camera.projection = Math::Perspective(main_camera.fov, aspect_ratio, main_camera.nearPlane, main_camera.farPlane);

    if (Math::isApproxZero(main_camera.fov - m_goal_fov))
        m_need_update = false;
}

void CameraManipulator::onKeyUpdate(int key, float frame_time)
{
    // 每一帧持续时间越长，意味着上一帧的渲染花费了越多时间，所以这一帧的速度应该越大，来平衡渲染所花去的时间
    float frame_speed = CameraMovementSpeed * frame_time;
    auto camera_forward = main_camera.direction;
    auto camera_right = main_camera.getRightDirection();
    auto upDirection = main_camera.upDirection;
    switch (key)
    {
    case 'W':
        main_camera.pos += camera_forward * frame_speed;
        break;
    case 'A':
        main_camera.pos -= camera_right * frame_speed;
        break;
    case 'D':
        main_camera.pos += camera_right * frame_speed;
        break;
    case 'S':
        main_camera.pos -= camera_forward * frame_speed;
        break;
    case 'Z':
        main_camera.pos += upDirection * frame_speed;
        break;
    case 'C':
        main_camera.pos -= upDirection * frame_speed;
        break;
    default:
        break;
    }
    main_camera.view = Math::LookAt(main_camera.pos, main_camera.pos + main_camera.direction, upDirection);
}

void CameraManipulator::onMouseUpdate(double delta_x, double delta_y, MouseButton mouse_button)
{
    if (main_camera.mode == Mode::Orbit)
    {
        if (mouse_button == MouseButton::Left)
        {
            // TODO 框选多选
        }
        if (mouse_button == MouseButton::Right)
        {
            auto rotate_Y = Math::Rotate(-(float)(0.3f * delta_x * RatationSensitivity), CameraComponent::global_up);
            main_camera.pos = rotate_Y * Vec4(main_camera.pos, 1.0f);
            main_camera.direction = rotate_Y * Vec4(main_camera.direction, 1.0f);
            main_camera.direction = Math::Normalize(main_camera.direction);
            main_camera.upDirection = Vec3(rotate_Y * Vec4(main_camera.upDirection, 1.0f));

            auto camera_right = main_camera.getRightDirection();
            auto rotate_x = Math::Rotate((float)(0.3f * delta_y * RatationSensitivity), camera_right);
            main_camera.pos = rotate_x * Vec4(main_camera.pos, 1.0f);
            main_camera.direction = rotate_x * Vec4(main_camera.direction, 1.0f);
            main_camera.upDirection = Vec3(rotate_x * Vec4(main_camera.upDirection, 1.0f));

            main_camera.view = Math::LookAt(main_camera.pos, main_camera.pos + main_camera.direction, main_camera.upDirection);
        }
        else if (mouse_button == MouseButton::Middle)
        {
            float coef = tan(main_camera.fov / 2.0f) / tan(main_camera.originFov / 2.0f);
            main_camera.pos += -(float)(delta_x * PanSensitivity) * coef * main_camera.getRightDirection();
            main_camera.pos += -(float)(delta_y * PanSensitivity) * coef * main_camera.upDirection;

            main_camera.view = Math::LookAt(main_camera.pos, main_camera.pos + main_camera.direction, main_camera.upDirection);
        }
    }

    if (main_camera.mode == Mode::FPS)
    {
        if (mouse_button == MouseButton::Left)
        {
            // get pitch
            main_camera.fps_params.pitch += delta_y * RatationSensitivity;
            // get yaw
            main_camera.fps_params.yaw += delta_x * RatationSensitivity;

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            // Euler angle problem:
            if (main_camera.fps_params.pitch > 89.0f)
                main_camera.fps_params.pitch = 89.0f;
            if (main_camera.fps_params.pitch < -89.0f)
                main_camera.fps_params.pitch = -89.0f;

            // update direction
            main_camera.direction.x = cos(Math::deg2rad(main_camera.fps_params.pitch)) * sin(Math::deg2rad(main_camera.fps_params.yaw));
            main_camera.direction.y = sin(Math::deg2rad(main_camera.fps_params.pitch));
            main_camera.direction.z = -cos(Math::deg2rad(main_camera.fps_params.pitch)) * cos(Math::deg2rad(main_camera.fps_params.yaw));

            main_camera.upDirection.x = sin(Math::deg2rad(main_camera.fps_params.pitch)) * sin(Math::deg2rad(main_camera.fps_params.yaw));
            main_camera.upDirection.y = cos(Math::deg2rad(main_camera.fps_params.pitch));
            main_camera.upDirection.z = -sin(Math::deg2rad(main_camera.fps_params.pitch)) * cos(Math::deg2rad(main_camera.fps_params.yaw));

            main_camera.view = Math::LookAt(main_camera.pos, main_camera.pos + main_camera.direction, main_camera.upDirection);
        }
    }
}

void CameraManipulator::orbitRotate(Vec3 start, Vec3 end)
{
    // 计算旋转角度
    float angle = acos(fmin(1.0f, Math::Dot(start, end)));
    // 计算旋转轴
    Vec3 rotate_axis = Math::Normalize(Math::Cross(start, end));
    // Vec3 world_rotate_axis = Inverse(Mat3(main_camera.view)) * rotate_axis;

    Mat4 rotate_mat = Math::Rotate(angle, rotate_axis);

    main_camera.pos = rotate_mat * Vec4(main_camera.pos, 1.0f);
    main_camera.direction = rotate_mat * Vec4(main_camera.direction, 1.0f);
    main_camera.upDirection = Vec3(rotate_mat * Vec4(main_camera.upDirection, 1.0f));
    main_camera.view = Math::LookAt(main_camera.pos, main_camera.pos + main_camera.direction, main_camera.upDirection);
}

void CameraManipulator::onMouseWheelUpdate(double yoffset, double mouse_x, double mouse_y)
{
    if (main_camera.zoom_mode == ZoomMode::ZoomToCenter)
    {
        m_goal_fov = 2 * atan(tan(m_goal_fov / 2.f) / (1 + ZoomUnit * (float)yoffset));
        if (m_goal_fov <= Math::deg2rad(0.01f))
            m_goal_fov = Math::deg2rad(0.01f);
        if (m_goal_fov >= Math::deg2rad(135.0f))
            m_goal_fov = Math::deg2rad(135.0f);

        m_need_update = true;
    }
    if (main_camera.zoom_mode == ZoomMode::ZoomToMouse)
    {
        Vec3 mouse_3d_pos = rayCastPlaneZero(mouse_x, mouse_y);

        // Logger::debug("Mouse Ray");
        // Logger::debug("Mouse 2d position: {},{}", mouse_x, mouse_y);
        // Logger::debug("Mouse 3d position: {},{},{}", mouse_3d_pos.x, mouse_3d_pos.y, mouse_3d_pos.z);
        // Logger::debug("\n");

        float viewport_width = (float)m_viewport.width;
        float viewport_height = (float)m_viewport.height;
        Vec3 center_3d_pos = rayCastPlaneZero(viewport_width / 2.0f, viewport_height / 2.0f);
        Vec3 displacement = mouse_3d_pos - center_3d_pos;

        if (yoffset == 0.0)
            return;
        //// 1. first translate to mouse_3d_pos
        // main_camera.pos += displacement;
        // float old_zoom = main_camera.zoom;

        //// 2. set zoom
        // main_camera.zoom += ZoomUnit * (float)yoffset;
        // if (main_camera.zoom < 0.1f)
        //     main_camera.zoom = 0.1f;

        // main_camera.fov = main_camera.originFov / main_camera.zoom;
        // if (main_camera.fov <= Math::deg2rad(0.01f))
        //     main_camera.fov = Math::deg2rad(0.01f);
        // if (main_camera.fov >= Math::deg2rad(135.0f))
        //     main_camera.fov = Math::deg2rad(135.0f);

        // 3. second translate back to original pos
        // main_camera.pos -= displacement * (old_zoom / main_camera.zoom);

        // 4. set view matrix, projection matrix
        main_camera.view = Math::LookAt(main_camera.pos, main_camera.pos + main_camera.direction, main_camera.upDirection);
        float aspect_ratio = m_viewport.AspectRatio();
        main_camera.projection = Math::Perspective(main_camera.fov, aspect_ratio, main_camera.nearPlane, main_camera.farPlane);
    }
}

Vec3 CameraManipulator::rayCastPlaneZero(double mouse_x, double mouse_y)
{

    // 1.  get ray direction from mouse position
    Vec3 cam_right = main_camera.getRightDirection();
    float viewport_width = (float)m_viewport.width;
    float viewport_height = (float)m_viewport.height;
    // normalized the x, y coordinate and take the viewport center as origin
    float u = 2.0f * mouse_x / viewport_width - 1.0f;
    float v = 2.0f * mouse_y / viewport_height - 1.0f;
    v = -v;

    float tangent = std::tan(main_camera.fov / 2.0f);
    Vec3 ray_direction = main_camera.direction + cam_right * tangent * u * (m_viewport.AspectRatio()) + main_camera.upDirection * tangent * v;
    ray_direction = Math::Normalize(ray_direction);
    // 2.  solve the intersection equation of the ray and the plane:
    // plane_normal. Dot(m_position + t * ray_direction - p0) = 0
    //`Vec3 plane_normal = Vec3(0, 1, 0);
    Vec3 plane_normal = -main_camera.direction;
    Vec4 zero_plane = Vec4(plane_normal.x, plane_normal.y, plane_normal.z, 0);
    Vec3 p0 = plane_normal * zero_plane[3];
    float t = (Math::Dot(plane_normal, p0) - Math::Dot(plane_normal, main_camera.pos) / Math::Dot(plane_normal, ray_direction));
    return Vec3(main_camera.pos + t * ray_direction);
}