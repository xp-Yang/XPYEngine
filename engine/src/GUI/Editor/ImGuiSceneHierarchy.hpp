#ifndef ImGuiSceneHierarchy_hpp
#define ImGuiSceneHierarchy_hpp

#include "Base/Common.hpp"

class GObject;
class ImGuiEditor;
class ImGuiSceneHierarchy {
public:
	ImGuiSceneHierarchy(ImGuiEditor* parent);
	void render();

private:
	std::unordered_map<std::string, std::function<void(std::string, const Meta::Instance&)>> m_widget_creator;
	ImGuiEditor* m_parent;
};

#endif
