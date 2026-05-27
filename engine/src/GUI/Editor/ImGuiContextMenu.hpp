#ifndef ImGuiContextMenu_hpp
#define ImGuiContextMenu_hpp

#include "Base/Common.hpp"

enum class ContextType
{
	Void,
	Object,
};

class ImGuiEditor;
class ImGuiContextMenu
{
public:
	ImGuiContextMenu(ImGuiEditor *parent) : m_parent(parent) {}
	void render();
	void popUp(ContextType context);

protected:
	bool m_menu_opened{false};
	ContextType m_context{ContextType::Void};

	ImGuiEditor *m_parent;
};

#endif
