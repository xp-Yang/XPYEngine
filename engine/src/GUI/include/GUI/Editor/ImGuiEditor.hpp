#ifndef ImGuiEditor_hpp
#define ImGuiEditor_hpp

#include "GUI/Viewport.hpp"
#include <functional>
#include <string>

class MainCanvas;
class ImGuiContextMenu;
class ImGuiSceneHierarchy;
class ImGuiGlobalConsole;
class ImGuiDebugWindow;
class ImGuiEditor {
public:
	using ExternalOpenFileHandler = std::function<bool(const std::string&)>;

	ImGuiEditor();
	~ImGuiEditor();
	void onUpdate();
	void beginFrame();
	void endFrame();
	Viewport getMainViewport() const;
	void popUpMenu();
	void setExternalOpenFileHandler(ExternalOpenFileHandler handler);

protected:
	void renderMenuBar();
	void renderEmptyMainDockerSpaceWindow();
	void configUIStyle();

private:
	std::unique_ptr<MainCanvas> m_main_canvas;
	std::unique_ptr<ImGuiContextMenu> m_context_menu;
	std::unique_ptr<ImGuiSceneHierarchy> m_scene_hierarchy_window;
	std::unique_ptr<ImGuiGlobalConsole> m_global_console_window;
	std::unique_ptr<ImGuiDebugWindow> m_debug_window;

	bool m_show_debug{ false };
	ExternalOpenFileHandler m_external_open_file_handler;
};

#endif
