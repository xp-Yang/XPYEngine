#ifndef ImGuiToolbar_hpp
#define ImGuiToolbar_hpp

#include "Base/Common.hpp"
#include "Render/RHI/rhi.hpp"

enum class ToolbarType : int {
    Translate,
    Rotate,
    Scale,
};

class Scene;
class ImGuiCanvas;
class ImGuiToolbar {
public:
    ImGuiToolbar(ImGuiCanvas* parent, std::shared_ptr<Scene> scene);
    void render();

protected:
    void renderToolbar();
    void renderGizmos();
    ToolbarType m_toolbar_type{ ToolbarType::Translate };
    ImGuiCanvas* m_parent{ nullptr };

    GL_HANDLE m_tranlate_icon_id{ 0 };
    GL_HANDLE m_rotate_icon_id{ 0 };
    GL_HANDLE m_scale_icon_id{ 0 };

    std::shared_ptr<Scene> ref_scene;
};

#endif
