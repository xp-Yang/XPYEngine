#ifndef ImGuiCanvas_hpp
#define ImGuiCanvas_hpp

#include "Base/Common.hpp"
#include "GUI/Viewport.hpp"

struct ImGuiWindow;

enum CanvasType : unsigned int {
    Main,
};

class ImGuiEditor;
class ImGuiCanvas {
public:
    ImGuiCanvas(ImGuiEditor* parent) : m_parent(parent) {}
    virtual void render() = 0;
    void setViewPort(const Viewport& viewport) { m_viewport = viewport; }
    Viewport getViewport() const { return m_viewport; }
    CanvasType type() const { return m_type; }
    ImGuiEditor* parent() const { return m_parent; }
    ImGuiWindow* getImGuiWindow() const { return m_imgui_window; }

protected:
    CanvasType m_type;
    Viewport m_viewport;

    ImGuiEditor* m_parent{ nullptr };
    ImGuiWindow* m_imgui_window{ nullptr };
};

class ImGuiToolbar;
class MainCanvas : public ImGuiCanvas {
public:
    MainCanvas(ImGuiEditor* parent) : ImGuiCanvas(parent) { m_type = CanvasType::Main; }
    void render() override;

protected:
    ImGuiToolbar* m_toolbar{ nullptr };
};

#endif
