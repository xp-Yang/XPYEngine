#ifndef CameraComponent_hpp
#define CameraComponent_hpp

#include "Logical/Framework/Component/Component.hpp"

enum class Mode
{
	Orbit,
	FPS,
};

enum class Projection
{
	Perspective,
	Ortho,
};

enum class ZoomMode
{
	ZoomToCenter,
	ZoomToMouse,
};

struct CameraComponent : public Component
{
	inline static Vec3 global_up = Vec3(0.0f, 1.0f, 0.0f); // vec3(0.0f, 1.0f, 0.0f) (y为上) or vec3(0.0f, 0.0f, 1.0f) (z为上)

	CameraComponent(GObject *parent) : Component(parent) {}
	CameraComponent(const CameraComponent &) = delete;
	CameraComponent &operator=(const CameraComponent &) = delete;

	Mode mode{Mode::Orbit};

	Projection projection_mode{Projection::Perspective};

	ZoomMode zoom_mode{ZoomMode::ZoomToCenter};

	// FPS style
	// 欧拉角表示：
	struct FPSParams
	{
		float pitch = 0.0f; // 方向向量与世界坐标系 x-z 平面的夹角
		float yaw = 0.0f;	// 方向向量在世界坐标系 x-z 平面的投影矢量相对世界坐标系 -z 轴的夹角
		float roll = 0.0f;
	} fps_params;

	float originFov = Math::deg2rad(45.0f); // 竖直fov

	float fov = originFov;
	float nearPlane = 0.1f;
	float farPlane = 1000.0f;
	Vec3 direction = Math::Normalize(Vec3(0.0f, -0.6f, -0.8f));
	Vec3 upDirection = Math::Normalize(global_up - Math::Dot(global_up, direction) * direction); // camera 的 y 轴
	Vec3 getRightDirection() const
	{ // camera 的 x 轴
		return Math::Cross(direction, upDirection);
	}
	Vec3 pos = Vec3(0.0f) - 115.0f * direction;
	Mat4 view = Math::LookAt(pos, pos + direction, global_up);
	float aspectRatio{16.0f / 9.0f}; // response on window size change, by sync_camera_projection_to_content()
	Mat4 projection = projection_mode == Projection::Perspective ? Math::Perspective(fov, aspectRatio, nearPlane, farPlane) : Math::Ortho(-15.0f * aspectRatio, 15.0f * aspectRatio, -15.0f, 15.0f, nearPlane, farPlane);
};

#endif // !CameraComponent_hpp
