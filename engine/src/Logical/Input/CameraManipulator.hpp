#ifndef CameraManipulator_hpp
#define CameraManipulator_hpp

#include "Base/Math/Rect.hpp"
#include "Base/Math/Math.hpp"
#include "Logical/Input/InputEnums.hpp"

struct CameraComponent;

class CameraManipulator{
public:
	CameraManipulator(CameraComponent& camera_);
	void syncContext(const IntRect& view_rect);
	void onUpdate();
	void onKeyUpdate(int key, float frame_time);
	void onMouseUpdate(double delta_x, double delta_y, MouseButton mouse_button);
	void orbitRotate(Vec3 start, Vec3 end);
	void onMouseWheelUpdate(double yoffset, double mouse_x, double mouse_y);
	Vec3 rayCastPlaneZero(double mouse_x, double mouse_y);
	bool isBoxSelectionEnabled() const;
	void selectObjectsInRect(const Vec2& start, const Vec2& end, bool retain_old = false);

	inline static const float CameraMovementSpeed = 20.0f;
	inline static const float RatationSensitivity = 0.01f;
	inline static const float PanSensitivity = 0.135f;
	inline static const float ZoomUnit = 0.1f;

protected:
	CameraComponent& camera;

	IntRect m_view_rect;

	float m_goal_fov;

	bool m_need_update{ false };
};

#endif
