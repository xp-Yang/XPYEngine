#ifndef ImGuiCanvas_hpp
#define ImGuiCanvas_hpp

#include "Base/Common.hpp"
#include "Base/Math/Rect.hpp"

struct ImGuiWindow;

enum CanvasType : unsigned int {
    Main,
};

class ImGuiEditor;
class ImGuiCanvas {
public:
    ImGuiCanvas(ImGuiEditor* parent) : m_parent(parent) {}
    virtual void render() = 0;
    void setCanvasRect(const IntRect& canvas_rect) { m_canvas_rect = canvas_rect; }
    IntRect canvasRect() const { return m_canvas_rect; }
    CanvasType type() const { return m_type; }
    ImGuiEditor* parent() const { return m_parent; }
    ImGuiWindow* getImGuiWindow() const { return m_imgui_window; }

protected:
    CanvasType m_type;
    IntRect m_canvas_rect;

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
