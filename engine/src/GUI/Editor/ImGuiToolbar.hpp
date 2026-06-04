#ifndef ImGuiToolbar_hpp
#define ImGuiToolbar_hpp

#include "Base/Common.hpp"
#include "Render/RHI/rhi.hpp"

#include <memory>

namespace Snapshot {
class Transaction;
}

enum class ToolbarType : int {
    Translate,
    Rotate,
    Scale,
};

class Scene;
struct RenderTextureResource;
class ImGuiCanvas;
class ImGuiToolbar {
public:
    ImGuiToolbar(ImGuiCanvas* parent, std::shared_ptr<Scene> scene);
    ~ImGuiToolbar();
    void render();

protected:
    void renderToolbar();
    void renderGizmos();
    ToolbarType m_toolbar_type{ ToolbarType::Translate };
    ImGuiCanvas* m_parent{ nullptr };

    GL_HANDLE m_tranlate_icon_id{ 0 };
    GL_HANDLE m_rotate_icon_id{ 0 };
    GL_HANDLE m_scale_icon_id{ 0 };
    std::shared_ptr<RenderTextureResource> m_tranlate_icon_texture;
    std::shared_ptr<RenderTextureResource> m_rotate_icon_texture;
    std::shared_ptr<RenderTextureResource> m_scale_icon_texture;

    std::shared_ptr<Scene> ref_scene;
    std::unique_ptr<Snapshot::Transaction> m_gizmo_transaction;
    bool m_gizmo_was_using{ false };
};

#endif
