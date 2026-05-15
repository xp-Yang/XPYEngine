#include "GUI/Editor/ImGuiDebugWindow.hpp"

#include <algorithm>
#include <cstdint>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>

#include "Logical/Framework/World/Scene.hpp"
#include "Render/RenderSystem.hpp"
#include "GUI/Editor/ImGuiEditor.hpp"
#include "GlobalContext.hpp"

namespace {

ImTextureID toImTextureID(unsigned int texture_id)
{
    return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture_id));
}

int resourceColumnCount(float content_width)
{
    if (content_width > 1100.0f)
        return 3;
    if (content_width > 640.0f)
        return 2;
    return 1;
}

void renderRenderGraphResources(RenderSystem* render_system)
{
    ImGui::Begin("RenderGraph Resources", nullptr, ImGuiWindowFlags_NoCollapse);

    if (!render_system)
    {
        ImGui::TextDisabled("RenderSystem unavailable");
        ImGui::End();
        return;
    }

    const std::vector<std::string> resource_names = render_system->renderGraphResourceNames();
    if (resource_names.empty())
    {
        ImGui::TextDisabled("No RenderGraph resources compiled yet");
        ImGui::End();
        return;
    }

    const float content_width = ImGui::GetContentRegionAvail().x;
    const int columns = resourceColumnCount(content_width);
    if (ImGui::BeginTable("RenderGraphResourceTable", columns, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable))
    {
        for (const std::string& resource_name : resource_names)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(resource_name.c_str());
            ImGui::TextUnformatted(resource_name.c_str());

            const unsigned int texture_id = render_system->renderGraphTextureOf(resource_name);
            const float image_width = std::max(1.0f, ImGui::GetContentRegionAvail().x);
            const float image_height = std::clamp(image_width * 0.5625f, 96.0f, 280.0f);
            if (texture_id != 0)
                ImGui::Image(toImTextureID(texture_id), ImVec2(image_width, image_height), ImVec2(0, 1), ImVec2(1, 0));
            else
                ImGui::TextDisabled("<no texture>");

            ImGui::Spacing();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void renderRenderGraphExecution(RenderSystem* render_system)
{
    ImGui::Begin("RenderGraph Execution", nullptr, ImGuiWindowFlags_NoCollapse);

    if (!render_system)
    {
        ImGui::TextDisabled("RenderSystem unavailable");
        ImGui::End();
        return;
    }

    const std::string execution_order = render_system->renderGraphExecutionDump();
    ImGui::TextUnformatted("Execution Order");
    ImGui::Separator();
    ImGui::TextUnformatted(execution_order.empty() ? "<empty>" : execution_order.c_str());

    if (ImGui::CollapsingHeader("Graph Dump"))
    {
        const std::string graph_dump = render_system->renderGraphDebugDump();
        ImGui::TextUnformatted(graph_dump.empty() ? "<empty>" : graph_dump.c_str());
    }

    ImGui::End();
}

void renderMainCameraInfo()
{
    ImGui::Begin("Main Camera Info", nullptr, ImGuiWindowFlags_NoCollapse);

    if (!g_context.scene)
    {
        ImGui::TextDisabled("Scene unavailable");
        ImGui::End();
        return;
    }

    auto& camera = g_context.scene->getMainCamera();
    ImGui::NewLine();
    ImGui::TextUnformatted("view matrix:");
    const std::string view = Utils::mat4ToStr(camera.view);
    ImGui::TextUnformatted(view.c_str());
    ImGui::NewLine();
    ImGui::TextUnformatted("inverse view matrix:");
    const std::string inverse_view = Utils::mat4ToStr(Math::Inverse(camera.view));
    ImGui::TextUnformatted(inverse_view.c_str());
    ImGui::NewLine();
    ImGui::TextUnformatted("camera position:");
    const std::string camera_pos = Utils::vec3ToStr(camera.pos);
    ImGui::TextUnformatted(camera_pos.c_str());
    ImGui::NewLine();
    ImGui::TextUnformatted("camera direction:");
    const std::string camera_dir = Utils::vec3ToStr(camera.direction);
    ImGui::TextUnformatted(camera_dir.c_str());
    ImGui::End();
}

} // namespace

ImGuiDebugWindow::ImGuiDebugWindow(ImGuiEditor* parent)
    : m_parent(parent)
{
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
        ImGui::DockBuilderDockWindow("RenderGraph Resources", debug_dock_id);
        ImGui::DockBuilderDockWindow("RenderGraph Execution", debug_dock_id);

        ImGui::DockBuilderFinish(debug_dock_id);
    }
    ImGui::DockSpace(debug_dock_id);
    ImGui::End();

    RenderSystem* render_system = g_context.render_system.get();
    renderRenderGraphResources(render_system);
    renderRenderGraphExecution(render_system);
    renderMainCameraInfo();
}
