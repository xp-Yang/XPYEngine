#include "GUI/Editor/ImGuiCanvas.hpp"
#include "GUI/Editor/ImGuiToolbar.hpp"
#include "GUI/Editor/ImGuiEditor.hpp"
#include <cstdint>
#include <imgui.h>
#include <imgui_internal.h>

#if ENABLE_ECS
#include "Logical/FrameWork/ECS/World.hpp"
#include "Logical/FrameWork/ECS/Components.hpp"
#endif

#include "Render/Graph/RenderGraph.hpp"
#include "Render/RenderSystem.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "GlobalContext.hpp"

static ImTextureID to_imgui_texture_id(unsigned int texture_id)
{
    return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture_id));
}

static void sync_camera_projection_to_content(const ImVec2& content_size)
{
    if (!g_context.scene || content_size.x <= 0.0f || content_size.y <= 0.0f) return;

    CameraComponent& camera = g_context.scene->getMainCamera();
    const float aspect_ratio = content_size.x / content_size.y;
    camera.aspectRatio = aspect_ratio;
    camera.projection = camera.projection_mode == Projection::Perspective
        ? Math::Perspective(camera.fov, aspect_ratio, camera.nearPlane, camera.farPlane)
        : Math::Ortho(-15.0f * aspect_ratio, 15.0f * aspect_ratio, -15.0f, 15.0f, camera.nearPlane, camera.farPlane);
}

void MainCanvas::render()
{
    if (!m_toolbar) {
        m_toolbar = new ImGuiToolbar(this, g_context.scene);
    }

    auto render_system = g_context.render_system;
    static ImGuiWindowFlags window_flags = 0;
    ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_Appearing);
    if (ImGui::Begin("MainCanvas", nullptr, window_flags | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground)) {
        m_imgui_window = ImGui::GetCurrentWindow();
        bool hovered_window = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(m_imgui_window->InnerRect.Min, m_imgui_window->InnerRect.Max);
        window_flags = hovered_window ? ImGuiWindowFlags_NoMove : 0;
        ImVec2 window_pos = ImGui::GetWindowPos();
        ImVec2 window_size = ImGui::GetWindowSize();
        ImVec2 content_size = ImGui::GetContentRegionAvail();
        ImVec2 content_pos = ImVec2(ImGui::GetWindowContentRegionMin().x + window_pos.x, ImGui::GetWindowContentRegionMin().y + window_pos.y);
        sync_camera_projection_to_content(content_size);
        ImTextureID scene_tex_id = to_imgui_texture_id(render_system->renderGraphTextureOf(RGResource::FinalColor));
        ImGui::SetCursorScreenPos(content_pos);
        ImGui::Image(scene_tex_id, content_size, ImVec2(0, 1), ImVec2(1, 0)); // fill content region
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsItemHovered())
            ImGui::CaptureMouseFromApp(false);
        ImGui::SetCursorScreenPos(content_pos);
        ImGui::Text("FPS %.1f", io.Framerate);
        setViewPort({ (int)content_pos.x, (int)content_pos.y, (int)content_size.x, (int)content_size.y });

        m_toolbar->render();
    }
    ImGui::End();
}
