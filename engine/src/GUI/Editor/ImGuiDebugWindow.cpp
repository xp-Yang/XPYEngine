#include "GUI/Editor/ImGuiDebugWindow.hpp"

#include <algorithm>
#include <cstdint>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>

#include "Logical/Framework/World/Scene.hpp"
#include "Render/RenderPipelineLibrary.hpp"
#include "Render/RenderSystem.hpp"
#include "GUI/Editor/ImGuiEditor.hpp"
#include "GlobalContext.hpp"

namespace {

struct ResourceViewerState {
    std::string selected_resource;
    bool open{ false };
    float depth_min{ 0.0f };
    float depth_max{ 1.0f };
};

ResourceViewerState& resourceViewerState()
{
    static ResourceViewerState state;
    return state;
}

ImTextureID toImTextureID(GL_HANDLE texture_id)
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

std::string joinStrings(const std::vector<std::string>& values)
{
    std::string result;
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            result += " -> ";
        result += values[i];
    }
    return result;
}

ImVec2 imageFitSize(const Vec2& texture_size, const ImVec2& max_size)
{
    const float source_width = std::max(1.0f, texture_size.x);
    const float source_height = std::max(1.0f, texture_size.y);
    const float aspect = source_width / source_height;

    ImVec2 size(std::max(1.0f, max_size.x), std::max(1.0f, max_size.x / aspect));
    if (size.y > max_size.y)
    {
        size.y = std::max(1.0f, max_size.y);
        size.x = std::max(1.0f, size.y * aspect);
    }
    return size;
}

class DebugTexturePreviewRenderer {
public:
    GL_HANDLE render(GL_HANDLE source_texture, bool remap_depth, float depth_min, float depth_max, int width, int height)
    {
        if (source_texture == 0 || width <= 0 || height <= 0)
            return 0;

        GLint previous_framebuffer = 0;
        GLint previous_viewport[4] = {};
        GLint previous_program = 0;
        GLint previous_vertex_array = 0;
        GLint previous_active_texture = 0;
        GLint previous_texture = 0;
        const GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean blend_enabled = glIsEnabled(GL_BLEND);

        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_framebuffer);
        glGetIntegerv(GL_VIEWPORT, previous_viewport);
        glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previous_vertex_array);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);

        ensure(width, height);
        if (m_framebuffer == 0 || m_texture == 0)
        {
            glBindTexture(GL_TEXTURE_2D, previous_texture);
            glActiveTexture(previous_active_texture);
            glBindFramebuffer(GL_FRAMEBUFFER, previous_framebuffer);
            glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);
            return 0;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, width, height);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        RhiGraphicsPipeline* preview_pipeline = RenderPipelineLibrary::graphicsPipeline(ShaderType::DebugTexturePreviewShader);
        glUseProgram(preview_pipeline->id());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, source_texture);
        glUniform1i(glGetUniformLocation(preview_pipeline->id(), "debugTexture"), 0);
        glUniform1i(glGetUniformLocation(preview_pipeline->id(), "remapDepth"), remap_depth ? 1 : 0);
        glUniform1f(glGetUniformLocation(preview_pipeline->id(), "depthMin"), depth_min);
        glUniform1f(glGetUniformLocation(preview_pipeline->id(), "depthMax"), depth_max);

        glBindVertexArray(m_vertex_array);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(previous_vertex_array);
        glUseProgram(previous_program);
        glBindTexture(GL_TEXTURE_2D, previous_texture);
        glActiveTexture(previous_active_texture);
        if (depth_test_enabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (blend_enabled)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, previous_framebuffer);
        glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);

        return m_texture;
    }

private:
    void ensure(int width, int height)
    {
        if (m_vertex_array == 0)
            glGenVertexArrays(1, &m_vertex_array);

        if (m_framebuffer == 0)
            glGenFramebuffers(1, &m_framebuffer);

        if (m_texture == 0)
            glGenTextures(1, &m_texture);

        if (m_width == width && m_height == height)
            return;

        m_width = width;
        m_height = height;

        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
    }

    GL_HANDLE m_framebuffer{ 0 };
    GL_HANDLE m_texture{ 0 };
    GL_HANDLE m_vertex_array{ 0 };
    int m_width{ 0 };
    int m_height{ 0 };
};

DebugTexturePreviewRenderer& debugTexturePreviewRenderer()
{
    static DebugTexturePreviewRenderer renderer;
    return renderer;
}

const RenderGraphResourceDebugInfo* findResourceInfo(const std::vector<RenderGraphResourceDebugInfo>& infos, const std::string& name)
{
    auto it = std::find_if(infos.begin(), infos.end(),
        [&name](const RenderGraphResourceDebugInfo& info)
        {
            return info.name == name;
        });
    return it == infos.end() ? nullptr : &(*it);
}

void renderRenderGraphResources(RenderSystem* render_system, std::string& selected_resource, bool& resource_viewer_open, float& depth_min, float& depth_max)
{
    ImGui::Begin("RenderGraph Resources", nullptr, ImGuiWindowFlags_NoCollapse);

    if (!render_system)
    {
        ImGui::TextDisabled("RenderSystem unavailable");
        ImGui::End();
        return;
    }

    const std::vector<RenderGraphResourceDebugInfo> resource_infos = render_system->renderGraphResourceDebugInfos();
    if (resource_infos.empty())
    {
        ImGui::TextDisabled("No RenderGraph resources compiled yet");
        ImGui::End();
        return;
    }

    const float content_width = ImGui::GetContentRegionAvail().x;
    const int columns = resourceColumnCount(content_width);
    if (ImGui::BeginTable("RenderGraphResourceTable", columns, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_Resizable))
    {
        for (const RenderGraphResourceDebugInfo& resource_info : resource_infos)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(resource_info.name.c_str());
            ImGui::TextUnformatted(resource_info.name.c_str());
            ImGui::TextDisabled("%s / %s / %s", resource_info.last_modifier_pass.c_str(), resource_info.render_target.c_str(), resource_info.attachment.c_str());

            const float image_width = std::max(1.0f, ImGui::GetContentRegionAvail().x);
            const float image_height = std::clamp(image_width * 0.5625f, 96.0f, 280.0f);
            if (resource_info.texture_id != 0)
            {
                ImGui::Image(toImTextureID(resource_info.texture_id), ImVec2(image_width, image_height), ImVec2(0, 1), ImVec2(1, 0));
                if (ImGui::IsItemClicked())
                {
                    if (selected_resource != resource_info.name)
                    {
                        depth_min = 0.0f;
                        depth_max = 1.0f;
                    }
                    selected_resource = resource_info.name;
                    resource_viewer_open = true;
                }
            }
            else
                ImGui::TextDisabled("<no texture>");

            ImGui::Spacing();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void renderPassList(const char* label, const std::vector<std::string>& passes)
{
    ImGui::TextUnformatted(label);
    if (passes.empty())
    {
        ImGui::TextDisabled("<empty>");
        return;
    }
    const std::string joined = joinStrings(passes);
    ImGui::TextWrapped("%s", joined.c_str());
}

void renderRenderGraphResourceViewer(RenderSystem* render_system, std::string& selected_resource, bool& resource_viewer_open, float& depth_min, float& depth_max)
{
    if (!resource_viewer_open || selected_resource.empty())
        return;

    if (!render_system)
    {
        resource_viewer_open = false;
        selected_resource.clear();
        return;
    }

    const std::vector<RenderGraphResourceDebugInfo> resource_infos = render_system->renderGraphResourceDebugInfos();
    const RenderGraphResourceDebugInfo* selected_info = findResourceInfo(resource_infos, selected_resource);
    if (!selected_info)
    {
        resource_viewer_open = false;
        selected_resource.clear();
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(960, 720), ImGuiCond_Appearing);
    if (!ImGui::Begin("RenderGraph Resource Viewer", &resource_viewer_open, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted(selected_info->name.c_str());
    ImGui::Separator();
    ImGui::Text("Storage: %s / %s / %s", selected_info->owner_pass.c_str(), selected_info->render_target.c_str(), selected_info->attachment.c_str());
    ImGui::Text("Last Modified: %s", selected_info->last_modifier_pass.c_str());
    ImGui::Text("Format: %s x%d%s", selected_info->format.c_str(), selected_info->sample_count, selected_info->transient ? " transient" : "");
    ImGui::Text("Size: %.0f x %.0f", selected_info->size.x, selected_info->size.y);

    if (selected_info->is_depth)
    {
        ImGui::SeparatorText("Depth Range");
        ImGui::SliderFloat("Min", &depth_min, 0.0f, 1.0f, "%.5f");
        ImGui::SliderFloat("Max", &depth_max, 0.0f, 1.0f, "%.5f");
        if (depth_min > depth_max)
            std::swap(depth_min, depth_max);
        if (ImGui::Button("Reset"))
        {
            depth_min = 0.0f;
            depth_max = 1.0f;
        }
    }

    ImGui::SeparatorText("Preview");
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float preview_max_height = std::max(180.0f, available.y * 0.62f);
    const ImVec2 image_size = imageFitSize(selected_info->size, ImVec2(available.x, preview_max_height));
    GL_HANDLE preview_texture = selected_info->texture_id;

    if (selected_info->is_depth)
    {
        if (selected_info->sample_count > 1)
        {
            ImGui::TextDisabled("Depth remap preview does not support multisampled textures yet");
            preview_texture = 0;
        }
        else
        {
            preview_texture = debugTexturePreviewRenderer().render(
                selected_info->texture_id,
                true,
                depth_min,
                depth_max,
                std::max(1, (int)image_size.x),
                std::max(1, (int)image_size.y));
        }
    }

    if (preview_texture != 0)
        ImGui::Image(toImTextureID(preview_texture), image_size, ImVec2(0, 1), ImVec2(1, 0));
    else
        ImGui::TextDisabled("<no preview texture>");

    ImGui::SeparatorText("History");
    renderPassList("Direct History", selected_info->direct_history);
    ImGui::Spacing();
    renderPassList("Contributors", selected_info->contributors);

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
    ResourceViewerState& viewer_state = resourceViewerState();
    renderRenderGraphResources(render_system, viewer_state.selected_resource, viewer_state.open, viewer_state.depth_min, viewer_state.depth_max);
    renderRenderGraphResourceViewer(render_system, viewer_state.selected_resource, viewer_state.open, viewer_state.depth_min, viewer_state.depth_max);
    renderRenderGraphExecution(render_system);
    renderMainCameraInfo();
}
