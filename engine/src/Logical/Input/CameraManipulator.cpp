#include "CameraManipulator.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "GlobalContext.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <utility>
#include <vector>

namespace
{
    struct SelectionRect
    {
        Vec2 min{ 0.0f };
        Vec2 max{ 0.0f };

        bool contains(const Vec2& point) const
        {
            return min.x <= point.x && point.x <= max.x &&
                min.y <= point.y && point.y <= max.y;
        }
    };

    struct LocalBounds
    {
        Vec3 min{ 0.0f };
        Vec3 max{ 0.0f };
        bool valid{ false };
    };

    SelectionRect selectionRectOf(const Vec2& start, const Vec2& end)
    {
        return {
            glm::min(start, end),
            glm::max(start, end)
        };
    }

    LocalBounds localBoundsOfMesh(const Mesh& mesh)
    {
        const auto& vertices = mesh.vertices();
        const auto& indices = mesh.indices();
        if (vertices.empty())
            return {};

        LocalBounds bounds;
        bounds.min = Vec3(std::numeric_limits<float>::max());
        bounds.max = Vec3(std::numeric_limits<float>::lowest());

        auto includeVertex = [&bounds, &vertices](int vertex_index)
        {
            if (vertex_index < 0 || vertex_index >= static_cast<int>(vertices.size()))
                return;
            bounds.min = glm::min(bounds.min, vertices[vertex_index].position);
            bounds.max = glm::max(bounds.max, vertices[vertex_index].position);
            bounds.valid = true;
        };

        if (!indices.empty())
        {
            const int index_start = std::max(0, mesh.index_offset);
            const int requested_count = mesh.index_count > 0 ? mesh.index_count : static_cast<int>(indices.size()) - index_start;
            const int index_end = std::min(static_cast<int>(indices.size()), index_start + requested_count);
            for (int i = index_start; i < index_end; ++i)
                includeVertex(indices[i]);
        }

        if (!bounds.valid)
        {
            for (int i = 0; i < static_cast<int>(vertices.size()); ++i)
                includeVertex(i);
        }

        return bounds;
    }

    std::array<Vec3, 8> cornersOf(const LocalBounds& bounds)
    {
        return {
            Vec3(bounds.min.x, bounds.min.y, bounds.min.z),
            Vec3(bounds.max.x, bounds.min.y, bounds.min.z),
            Vec3(bounds.min.x, bounds.max.y, bounds.min.z),
            Vec3(bounds.max.x, bounds.max.y, bounds.min.z),
            Vec3(bounds.min.x, bounds.min.y, bounds.max.z),
            Vec3(bounds.max.x, bounds.min.y, bounds.max.z),
            Vec3(bounds.min.x, bounds.max.y, bounds.max.z),
            Vec3(bounds.max.x, bounds.max.y, bounds.max.z),
        };
    }

    bool projectToScreen(const Vec3& world_position, const Mat4& view_projection, const IntRect& view_rect, Vec2& screen_position)
    {
        const Vec4 clip = view_projection * Vec4(world_position, 1.0f);
        if (clip.w <= Math::Constant::epsilon)
            return false;

        const Vec3 ndc = Vec3(clip) / clip.w;
        if (ndc.z < -1.0f || ndc.z > 1.0f)
            return false;

        screen_position.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(view_rect.width);
        screen_position.y = (0.5f - ndc.y * 0.5f) * static_cast<float>(view_rect.height);
        return true;
    }

    bool meshObjectFullyInsideRect(GObject& object, const MeshComponent& mesh_component, const CameraComponent& camera, const IntRect& view_rect, const SelectionRect& selection_rect)
    {
        const auto* transform = object.getComponent<TransformComponent>();
        if (!transform)
            return false;

        bool has_bounds = false;
        const Mat4 object_transform = transform->transform();
        const Mat4 view_projection = camera.projection * camera.view;
        for (const auto& sub_mesh : mesh_component.sub_meshes)
        {
            if (!sub_mesh)
                continue;

            const LocalBounds bounds = localBoundsOfMesh(*sub_mesh);
            if (!bounds.valid)
                continue;

            has_bounds = true;
            const Mat4 sub_mesh_transform = Math::composeMatrix(sub_mesh->scale, sub_mesh->rotation, sub_mesh->translation);
            const Mat4 model = object_transform * sub_mesh_transform;
            for (const Vec3& corner : cornersOf(bounds))
            {
                const Vec4 world = model * Vec4(corner, 1.0f);
                Vec2 screen_position;
                if (!projectToScreen(Vec3(world) / world.w, view_projection, view_rect, screen_position))
                    return false;
                if (!selection_rect.contains(screen_position))
                    return false;
            }
        }
        return has_bounds;
    }

    bool pointObjectInsideRect(GObject& object, const CameraComponent& camera, const IntRect& view_rect, const SelectionRect& selection_rect)
    {
        const auto* transform = object.getComponent<TransformComponent>();
        if (!transform)
            return false;

        const Mat4 view_projection = camera.projection * camera.view;
        const Vec4 world = transform->transform() * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        Vec2 screen_position;
        return projectToScreen(Vec3(world) / world.w, view_projection, view_rect, screen_position) &&
            selection_rect.contains(screen_position);
    }
}

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
            // Box selection is driven by GUIInput because it owns mouse start/end positions.
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

bool CameraManipulator::isBoxSelectionEnabled() const
{
    return camera.mode == Mode::Orbit;
}

void CameraManipulator::selectObjectsInRect(const Vec2& start, const Vec2& end, bool retain_old)
{
    if (!g_context.scene || !isBoxSelectionEnabled())
        return;

    const SelectionRect selection_rect = selectionRectOf(start, end);
    std::vector<GObjectID> selected_ids;
    for (const auto& object : g_context.scene->getObjects())
    {
        if (!object || !object->visible())
            continue;

        bool selected = false;
        if (auto* mesh_component = object->getComponent<MeshComponent>())
            selected = meshObjectFullyInsideRect(*object, *mesh_component, camera, m_view_rect, selection_rect);
        else
            selected = pointObjectInsideRect(*object, camera, m_view_rect, selection_rect);

        if (selected)
            selected_ids.push_back(object->ID());
    }

    const std::vector<GObjectID> old_selected_ids = g_context.scene->getPickedObjectIDs();
    if (retain_old)
    {
        selected_ids.erase(std::remove_if(selected_ids.begin(), selected_ids.end(),
            [&old_selected_ids](GObjectID id)
            {
                return std::find(old_selected_ids.begin(), old_selected_ids.end(), id) != old_selected_ids.end();
            }), selected_ids.end());
        g_context.scene->onPickedChanged(std::move(selected_ids), {});
    }
    else
    {
        g_context.scene->onPickedChanged(std::move(selected_ids), old_selected_ids);
    }
}
