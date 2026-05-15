#ifndef ImGuiDebugWindow_hpp
#define ImGuiDebugWindow_hpp

#include "Base/Common.hpp"

class ImGuiEditor;
class ImGuiDebugWindow {
public:
	ImGuiDebugWindow(ImGuiEditor* parent);
	void render();

protected:
	ImGuiEditor* m_parent;
};

#endif
