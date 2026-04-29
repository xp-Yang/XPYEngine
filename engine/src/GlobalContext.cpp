#include "GlobalContext.hpp"
#include "GUI/Window.hpp"
#include "Logical/FrameWork/World/Scene.hpp"
#include "Render/RenderSystem.hpp"
#include "GUI/Editor/ImGuiEditor.hpp"
#include "GUI/Editor/ImGuiInput.hpp"
#include "Logical/Animation/AnimationSystem.hpp"
#include "AllMetaRegister.hpp"
#include "Project/ProjectManager.hpp"

GlobalContext g_context;

GlobalContext::GlobalContext()
{
	Meta::Registration::allMetaRegister();

	window = std::make_shared<Window>((int)DEFAULT_WINDOW_WIDTH, (int)DEFAULT_WINDOW_HEIGHT);
	scene = std::make_shared<Scene>();
	render_system = std::make_shared<RenderSystem>();
	gui_editor = std::make_shared<ImGuiEditor>();
	gui_input = std::make_shared<GUIInput>(gui_editor);
	animation_system = std::make_shared<AnimationSystem>();
	project_manager = std::make_shared<ProjectManager>();
}

GlobalContext::~GlobalContext()
{

}