#include "ImGuiInput.hpp"
#include "GUI/Editor/ImGuiEditor.hpp"
#include "Platform/Window.hpp"
#include "Render/Graph/RenderGraphPassNode.hpp"
#include "Render/RenderSystem.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "GlobalContext.hpp"
#include <algorithm>
#include <imgui.h>

GUIInput::GUIInput(std::shared_ptr<ImGuiEditor> editor)
	: ref_editor(editor)
	, m_pick_solver(std::make_unique<PickSolver>(editor))
	, m_camera_manipulator(std::make_shared<CameraManipulator>(g_context.scene->getMainCamera()))
{
}

bool GUIInput::refreshState()
{
	ImGuiIO& io = ImGui::GetIO();

	auto main_viewport = ref_editor->getMainViewport();
	if (!(main_viewport.x <= io.MousePos.x && io.MousePos.x <= main_viewport.x + main_viewport.width &&
		main_viewport.y <= io.MousePos.y && io.MousePos.y <= main_viewport.y + main_viewport.height) ||
		io.WantCaptureMouse)
		return false;

	m_mouse_button = MouseButton::None;
	if (io.MouseDown[0] || ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		m_mouse_button = MouseButton::Left;
	}
	else if (io.MouseDown[1] || ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		m_mouse_button = MouseButton::Right;
	}
	else if (io.MouseDown[2] || ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
		m_mouse_button = MouseButton::Middle;
	}

	m_mouse_state = MouseState::Moving;
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
		ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
		ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
		m_mouse_state = MouseState::Clicked;
	}
	else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) ||
		ImGui::IsMouseDragging(ImGuiMouseButton_Right) ||
		ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
		m_mouse_state = MouseState::Dragging;
	}
	else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
		ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
		ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
		m_mouse_state = MouseState::Holding;
	}
	else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) ||
		ImGui::IsMouseReleased(ImGuiMouseButton_Right) ||
		ImGui::IsMouseReleased(ImGuiMouseButton_Middle)) {
		m_mouse_state = MouseState::Released;
	}

	m_mouse_x = mapToMainCanvasWindow(Vec2(io.MousePos.x, io.MousePos.y)).x;
	m_mouse_y = mapToMainCanvasWindow(Vec2(io.MousePos.x, io.MousePos.y)).y;
	m_delta_mouse_x = m_mouse_x - m_last_mouse_x;
	m_delta_mouse_y = -(m_mouse_y - m_last_mouse_y);
	m_last_mouse_x = m_mouse_x;
	m_last_mouse_y = m_mouse_y;
	m_mouse_wheel = io.MouseWheel;
	memcpy(KeysDown, io.KeysDown, sizeof(KeysDown));
	//Logger::debug("delta_x: {}, delta_y: {}", m_delta_mouse_x, m_delta_mouse_y);
	return true;
}

bool GUIInput::onUpdate(float delta_time)
{
	m_camera_manipulator->syncContext(ref_editor->getMainViewport());
	m_camera_manipulator->onUpdate();

	if (!refreshState()) {
		return false;
	}

	if (m_last_mouse_state == MouseState::Holding && m_mouse_state == MouseState::Released) {
		m_pick_solver->onPicking(m_mouse_x, m_mouse_y, KeysDown[Key_LeftCtrl]);
		if (m_mouse_button == MouseButton::Right) {
			ref_editor->popUpMenu();
		}
	}

	if (m_mouse_state == MouseState::Dragging) {
		m_camera_manipulator->onMouseUpdate(m_delta_mouse_x, m_delta_mouse_y, m_mouse_button);
	}

	if (!Math::isApproxZero(m_mouse_wheel))
		m_camera_manipulator->onMouseWheelUpdate(m_mouse_wheel, m_mouse_x, m_mouse_y);

	if (KeysDown['W']) {
		m_camera_manipulator->onKeyUpdate('W', delta_time);
	}
	if (KeysDown['A']) {
		m_camera_manipulator->onKeyUpdate('A', delta_time);
	}
	if (KeysDown['S']) {
		m_camera_manipulator->onKeyUpdate('S', delta_time);
	}
	if (KeysDown['D']) {
		m_camera_manipulator->onKeyUpdate('D', delta_time);
	}

	m_last_mouse_state = m_mouse_state;

	return true;
}

Vec2 GUIInput::mapToMainCanvasWindow(const Vec2& value)
{
	Vec2 pos = value;
	auto main_viewport = ref_editor->getMainViewport();
	pos.x -= main_viewport.x;
	pos.y -= main_viewport.y;
	return pos;
}

void PickSolver::onPicking(float mouse_x, float mouse_y, bool retain_old)
{
	int x = (int)mouse_x;
	int y = (int)mouse_y;
	// map to picking framebuffer size (follows render resolution preset)
	auto main_viewport = ref_editor->getMainViewport();
	Vec2 render_size = g_context.render_system->renderParams().renderTargetPixels();
	const int fb_w = (int)render_size.x;
	const int fb_h = (int)render_size.y;
	if (fb_w <= 0 || fb_h <= 0)
		return;
	x *= fb_w / (float)main_viewport.width;
	y *= fb_h / (float)main_viewport.height;
	x = std::clamp(x, 0, fb_w - 1);
	y = std::clamp(y, 0, fb_h - 1);
	// in gl coordinate system, left-bottom is as origin
	y = fb_h - 1 - y;

	unsigned char data[4] = { 0,0,0,0 };
	if (!g_context.render_system->readRenderGraphPixelRGBAOf(RGResource::PickingColor, x, y, data))
		return;
	int picked_id = ((int)data[0] + ((int)data[1] << 8) + ((int)data[2] << 16)) / PickingColorIDFactor;
	const auto& scene_objects = g_context.scene->getObjects();
	auto it = std::find_if(scene_objects.begin(), scene_objects.end(), [picked_id](const std::shared_ptr<GObject>& obj) {
		return obj->ID().id == picked_id;
		});
	if (it != scene_objects.end())
		emit pickedChanged({ (*it)->ID() }, retain_old ? std::vector<GObjectID>() : g_context.scene->getPickedObjectIDs());
	else if (!retain_old)
		emit pickedChanged({}, g_context.scene->getPickedObjectIDs());

	Logger::debug("PickSolver::onPicking(), picking({}, {}), mouse({}, {}), picked_id:{}", x, y, mouse_x, mouse_y, picked_id);
}

PickSolver::PickSolver(std::shared_ptr<ImGuiEditor> editor)
	: ref_editor(editor)
{
	connect(this, &pickedChanged, g_context.scene.get(), &Scene::onPickedChanged);
}
