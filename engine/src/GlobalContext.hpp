#ifndef GlobalContext_hpp
#define GlobalContext_hpp

#include <memory>

class GUIInput;
class ImGuiEditor;
class Scene;
class RenderSystem;
class Window;
class AnimationSystem;

struct GlobalContext {
    GlobalContext();
    ~GlobalContext();

    std::shared_ptr<GUIInput> gui_input{ nullptr };
    std::shared_ptr<ImGuiEditor> gui_editor{ nullptr };
    std::shared_ptr<Scene> scene{ nullptr };
    std::shared_ptr<RenderSystem> render_system{ nullptr };
    std::shared_ptr<Window> window{ nullptr };
    std::shared_ptr<AnimationSystem> animation_system{ nullptr };
};

extern GlobalContext g_context;

#endif
