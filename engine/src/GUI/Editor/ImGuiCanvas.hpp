#ifndef ImGuiCanvas_hpp
#define ImGuiCanvas_hpp

#include "Base/Common.hpp"
#include "GUI/Viewport.hpp"

struct ImGuiWindow;

enum CanvasType : unsigned int {
    Main,
    Preview,
    Pick,
    Shadow,
    GBuffer,
    Lighting,
    Bloom,
    RayTracing,
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

class PreviewCanvas : public ImGuiCanvas {
public:
    PreviewCanvas(ImGuiEditor* parent);
    void render() override;
};

class PickingCanvas : public ImGuiCanvas {
public:
    PickingCanvas(ImGuiEditor* parent) : ImGuiCanvas(parent) { m_type = CanvasType::Pick; }
    void render() override;
};

class ShadowCanvas : public ImGuiCanvas {
public:
    ShadowCanvas(ImGuiEditor* parent) : ImGuiCanvas(parent) { m_type = CanvasType::Shadow; }
    void render() override;
};

class GBufferCanvas : public ImGuiCanvas {
public:
    GBufferCanvas(ImGuiEditor* parent) : ImGuiCanvas(parent) { m_type = CanvasType::GBuffer; }
    void render() override;
};

class LightingCanvas : public ImGuiCanvas {
public:
    LightingCanvas(ImGuiEditor* parent) : ImGuiCanvas(parent) { m_type = CanvasType::Lighting; }
    void render() override;
};

class BloomCanvas : public ImGuiCanvas {
public:
    BloomCanvas(ImGuiEditor* parent) : ImGuiCanvas(parent) { m_type = CanvasType::Bloom; }
    void render() override;
};

#endif