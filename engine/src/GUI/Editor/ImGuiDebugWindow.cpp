#include "GUI/Editor/ImGuiDebugWindow.hpp"
#include "GUI/Editor/ImGuiCanvas.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "Logical/Framework/World/Scene.hpp"
#include "Render/RenderSystem.hpp"
#include "GUI/Editor/ImGuiEditor.hpp"
#include "GlobalContext.hpp"

ImGuiDebugWindow::ImGuiDebugWindow(ImGuiEditor* parent)
    : m_parent(parent)
{
    m_picking_canvas = std::make_unique<PickingCanvas>(m_parent);
    m_shadow_canvas = std::make_unique<ShadowCanvas>(m_parent);
    m_gbuffer_canvas = std::make_unique<GBufferCanvas>(m_parent);
    m_lighting_canvas = std::make_unique<LightingCanvas>(m_parent);
    m_bloom_canvas = std::make_unique<BloomCanvas>(m_parent);
}

void ImGuiDebugWindow::render()
{
    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Appearing);
    ImGui::Begin("Debug Window", nullptr, 
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGuiID debug_dock_id = ImGui::GetID("Debug Dock");
    if (!ImGui::DockBuilderGetNode(debug_dock_id))
    {
        ImGui::DockBuilderRemoveNode(debug_dock_id);
        ImGui::DockBuilderAddNode(debug_dock_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodePos(debug_dock_id, ImGui::GetCurrentWindow()->Pos);
        ImGui::DockBuilderSetNodeSize(debug_dock_id, ImGui::GetCurrentWindow()->Size);

        ImGui::DockBuilderDockWindow("Main Camera Info", debug_dock_id);
        ImGui::DockBuilderDockWindow("ShadowCanvas", debug_dock_id);
        ImGui::DockBuilderDockWindow("PickingCanvas", debug_dock_id);
        ImGui::DockBuilderDockWindow("GBufferCanvas", debug_dock_id);
        ImGui::DockBuilderDockWindow("LightingCanvas", debug_dock_id);
        ImGui::DockBuilderDockWindow("BloomCanvas", debug_dock_id);

        ImGui::DockBuilderFinish(debug_dock_id);
    }
    ImGui::DockSpace(debug_dock_id);
    ImGui::End();

    m_picking_canvas->render();
    m_shadow_canvas->render();
    m_gbuffer_canvas->render();
    m_lighting_canvas->render();
    m_bloom_canvas->render();

    ImGui::Begin("Main Camera Info", nullptr, ImGuiWindowFlags_NoCollapse);
    auto& camera = g_context.scene->getMainCamera();
    ImGui::NewLine();
    ImGui::Text("view matrix:");
    std::string test_view = Utils::mat4ToStr(camera.view);
    ImGui::Text(test_view.c_str());
    ImGui::NewLine();
    ImGui::Text("inverse view matrix:");
    std::string inverse_view = Utils::mat4ToStr(Math::Inverse(camera.view));
    ImGui::Text(inverse_view.c_str());
    ImGui::NewLine();
    ImGui::Text("camera position:");
    std::string test_camera_pos = Utils::vec3ToStr(camera.pos);
    ImGui::Text(test_camera_pos.c_str());
    ImGui::NewLine();
    ImGui::Text("camera direction:");
    std::string test_camera_dir = Utils::vec3ToStr(camera.direction);
    ImGui::Text(test_camera_dir.c_str());
    ImGui::End();
}
