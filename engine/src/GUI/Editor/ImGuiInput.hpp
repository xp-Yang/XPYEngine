#ifndef ImGuiInput_hpp
#define ImGuiInput_hpp

#include "Logical/Input/InputEnums.hpp"
#include "Base/Common.hpp"
#include "Logical/Input/CameraManipulator.hpp"
#include <Logical/Framework/Object/GObject.hpp>

class ImGuiEditor;
class PickSolver;
class GUIInput {
public:
	GUIInput(std::shared_ptr<ImGuiEditor> editor);
	bool onUpdate(float delta_time);

protected:
	bool refreshState();
	Vec2 mapToMainCanvasWindow(const Vec2& value);

	MouseState m_last_mouse_state{ MouseState::None };
	MouseState m_mouse_state{ MouseState::None };
	MouseButton m_mouse_button{ MouseButton::None };
	float m_last_mouse_x;
	float m_last_mouse_y;
	float m_mouse_x;
	float m_mouse_y;
	float m_delta_mouse_x;
	float m_delta_mouse_y;
	float m_mouse_wheel;

	bool KeysDown[Key_COUNT];

	std::shared_ptr<CameraManipulator> m_camera_manipulator;
	std::unique_ptr<PickSolver> m_pick_solver;

	std::shared_ptr<ImGuiEditor> ref_editor;
};

class PickSolver {
public:
	PickSolver(std::shared_ptr<ImGuiEditor> editor);

signals:
	Signal<std::vector<GObjectID>, std::vector<GObjectID>> pickedChanged;

public slots:
	void onPicking(float mouse_x, float mouse_y, bool retain_old = false);

private:
	std::shared_ptr<ImGuiEditor> ref_editor;
};

#endif
