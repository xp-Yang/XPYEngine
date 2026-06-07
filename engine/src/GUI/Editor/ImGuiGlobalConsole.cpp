#include "ImGuiGlobalConsole.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cmath>
#include <imgui.h>
#include <imgui_internal.h>

#include "GUI/Editor/ImGuiEditor.hpp"
#include "Render/RenderSystem.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "Logical/Snapshot/ObjectSnapshotCommand.hpp"
#include "Logical/Snapshot/ObjectSnapshotService.hpp"
#include "Logical/Snapshot/UndoRedoStack.hpp"
#include "GlobalContext.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr float kDefaultWindowWidth = 420.0f;
constexpr float kContentMaxWidth = 520.0f;
constexpr float kLabelWidth = 150.0f;
constexpr float kControlWidth = 180.0f;
constexpr float kValueWidth = 64.0f;
constexpr float kChildIndent = 16.0f;

void pushSnapshotCommand(Scene& scene, const std::string& label, Snapshot::ObjectSnapshot before, Snapshot::ObjectSnapshot after)
{
    if (Snapshot::ObjectSnapshotService::equals(before, after))
        return;

    std::vector<Snapshot::ObjectSnapshot> before_snapshots;
    before_snapshots.push_back(std::move(before));
    std::vector<Snapshot::ObjectSnapshot> after_snapshots;
    after_snapshots.push_back(std::move(after));
    scene.undoRedoStack().pushExecuted(std::make_unique<Snapshot::ObjectSnapshotCommand>(
        label,
        std::move(before_snapshots),
        std::move(after_snapshots)));
}

std::shared_ptr<GObject> lastPointLightObject(Scene& scene)
{
    const auto& objects = scene.getObjects();
    for (auto it = objects.rbegin(); it != objects.rend(); ++it)
    {
        if (*it && (*it)->hasComponent<PointLightComponent>())
            return *it;
    }
    return nullptr;
}

const char* renderPathLabel(RenderPathType type)
{
    switch (type)
    {
    case RenderPathType::Forward:
        return "Forward";
    case RenderPathType::Deferred:
        return "Deferred";
    default:
        return "Unknown";
    }
}

const char* materialModelLabel(MaterialModel model)
{
    switch (model)
    {
    case MaterialModel::BlinnPhong:
        return "BlinnPhong";
    case MaterialModel::PBR:
        return "PBR";
    default:
        return "Unknown";
    }
}

const char* renderResolutionLabel(RenderResolutionPreset preset)
{
    switch (preset)
    {
    case RenderResolutionPreset::FullHD_1080p:
        return "1080p";
    case RenderResolutionPreset::QHD_1440p:
        return "2K";
    case RenderResolutionPreset::UHD_4K:
        return "4K";
    default:
        return "Unknown";
    }
}

float compactControlWidth()
{
    return std::min(kControlWidth, ImGui::GetContentRegionAvail().x);
}

std::string hiddenId(const char* prefix, const char* label)
{
    return std::string("##") + prefix + label;
}

bool beginControlRow(const char* label, int column_count = 2)
{
    const std::string table_id = std::string("ControlRow##") + label;
    if (!ImGui::BeginTable(table_id.c_str(), column_count, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX))
        return false;

    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, kLabelWidth);
    ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthFixed, kControlWidth);
    if (column_count > 2)
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, kValueWidth);

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1);
    return true;
}

void endControlRow()
{
    ImGui::EndTable();
}

bool DrawSectionHeader(const char* label)
{
    ImGui::Spacing();
    return ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
}

bool BeginSection(const char* label)
{
    return DrawSectionHeader(label);
}

void EndSection()
{
    ImGui::Spacing();
}

bool DrawComboRow(const char* label, int& selected, const char* const* options, int option_count)
{
    bool changed = false;
    if (option_count <= 0 || !beginControlRow(label))
        return false;

    const int display_index = std::clamp(selected, 0, option_count - 1);
    ImGui::SetNextItemWidth(compactControlWidth());
    const std::string id = hiddenId("combo", label);
    if (ImGui::BeginCombo(id.c_str(), options[display_index]))
    {
        for (int i = 0; i < option_count; ++i)
        {
            const bool is_selected = selected == i;
            if (ImGui::Selectable(options[i], is_selected))
            {
                selected = i;
                changed = true;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    endControlRow();
    return changed;
}

bool DrawToggleRow(const char* label, bool* value)
{
    if (!beginControlRow(label))
        return false;

    const std::string id = hiddenId("toggle", label);
    const bool changed = ImGui::Checkbox(id.c_str(), value);
    ImGui::SameLine();
    ImGui::TextDisabled(*value ? "On" : "Off");

    endControlRow();
    return changed;
}

bool DrawSliderFloatRow(const char* label, float* value, float min_value, float max_value, const char* format)
{
    if (!beginControlRow(label, 3))
        return false;

    ImGui::SetNextItemWidth(compactControlWidth());
    const std::string id = hiddenId("slider", label);
    const bool changed = ImGui::SliderFloat(id.c_str(), value, min_value, max_value, "");

    char value_text[32];
    std::snprintf(value_text, sizeof(value_text), format, *value);
    ImGui::TableSetColumnIndex(2);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(value_text);

    endControlRow();
    return changed;
}

bool DrawSliderIntRow(const char* label, int* value, int min_value, int max_value)
{
    if (!beginControlRow(label, 3))
        return false;

    ImGui::SetNextItemWidth(compactControlWidth());
    const std::string id = hiddenId("slider", label);
    const bool changed = ImGui::SliderInt(id.c_str(), value, min_value, max_value, "");

    char value_text[32];
    std::snprintf(value_text, sizeof(value_text), "%d", *value);
    ImGui::TableSetColumnIndex(2);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(value_text);

    endControlRow();
    return changed;
}

bool DrawSegmentedRow(const char* label, int& selected, const char* const* options, int option_count)
{
    bool changed = false;
    if (option_count <= 0 || !beginControlRow(label))
        return false;

    const float segment_width = std::max(72.0f, (compactControlWidth() - ImGui::GetStyle().ItemSpacing.x * (option_count - 1)) / option_count);
    for (int i = 0; i < option_count; ++i)
    {
        if (i > 0)
            ImGui::SameLine();

        const bool is_selected = selected == i;
        if (is_selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }

        const std::string id = std::string(options[i]) + "##" + label;
        if (ImGui::Button(id.c_str(), ImVec2(segment_width, 0.0f)) && !is_selected)
        {
            selected = i;
            changed = true;
        }

        if (is_selected)
            ImGui::PopStyleColor(2);
    }

    endControlRow();
    return changed;
}

void DrawStepperRow(const char* label, int value, bool& minus_pressed, bool& plus_pressed)
{
    minus_pressed = false;
    plus_pressed = false;
    if (!beginControlRow(label))
        return;

    ImGui::PushButtonRepeat(true);
    minus_pressed = ImGui::Button((std::string("-##") + label).c_str(), ImVec2(28.0f, 0.0f));
    ImGui::SameLine();
    ImGui::Text("%d", value);
    ImGui::SameLine();
    plus_pressed = ImGui::Button((std::string("+##") + label).c_str(), ImVec2(28.0f, 0.0f));
    ImGui::PopButtonRepeat();

    endControlRow();
}

void drawHeader(RenderParams& render_params)
{
    ImGui::TextUnformatted("Render Console");
    ImGui::SameLine();
    ImGui::TextDisabled(
        "%s | %s | %s",
        renderPathLabel(render_params.render_path_type),
        renderResolutionLabel(render_params.render_resolution),
        materialModelLabel(render_params.material_model));
    ImGui::Separator();
}

void createPointLight()
{
    if (!g_context.scene)
        return;

    GObject* object = g_context.scene->createPointLight();
    if (object)
    {
        Snapshot::ObjectSnapshot before;
        before.id = object->ID();
        before.existed = false;
        Snapshot::ObjectSnapshot after = Snapshot::ObjectSnapshotService::capture(*g_context.scene, object->ID());
        pushSnapshotCommand(*g_context.scene, "Create Point Light", std::move(before), std::move(after));
    }
}

void deleteLastPointLight()
{
    if (!g_context.scene)
        return;

    std::shared_ptr<GObject> object = lastPointLightObject(*g_context.scene);
    if (object)
    {
        const GObjectID id = object->ID();
        Snapshot::ObjectSnapshot before = Snapshot::ObjectSnapshotService::capture(*g_context.scene, id);
        g_context.scene->removeObject(id);
        Snapshot::ObjectSnapshot after = Snapshot::ObjectSnapshotService::capture(*g_context.scene, id);
        pushSnapshotCommand(*g_context.scene, "Delete Point Light", std::move(before), std::move(after));
    }
}
}

void ImGuiGlobalConsole::render()
{
    ImGui::SetNextWindowSize(ImVec2(kDefaultWindowWidth, 640.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoCollapse);

    auto& render_params = g_context.render_system->renderParams();
    const float content_width = std::max(1.0f, std::min(ImGui::GetContentRegionAvail().x, kContentMaxWidth));
    ImGui::BeginChild("ConsoleContent", ImVec2(content_width, 0.0f), false);

    drawHeader(render_params);

    if (BeginSection("Renderer"))
    {
        static const char* render_path_options[] = { "Forward", "Deferred" };
        int path_type_option = static_cast<int>(render_params.render_path_type);
        if (DrawComboRow("Render Path", path_type_option, render_path_options, static_cast<int>(std::size(render_path_options))))
            render_params.render_path_type = static_cast<RenderPathType>(path_type_option);

        static const char* resolution_options[] = { "1080p (1920x1080)", "2K (2560x1440)", "4K (3840x2160)" };
        int resolution_option = static_cast<int>(render_params.render_resolution);
        if (DrawComboRow("Resolution", resolution_option, resolution_options, static_cast<int>(std::size(resolution_options))))
        {
            render_params.render_resolution = static_cast<RenderResolutionPreset>(resolution_option);
            g_context.render_system->rebuildRenderTargets();
        }

        static const char* material_options[] = { "BlinnPhong", "PBR" };
        int material_option = static_cast<int>(render_params.material_model);
        if (DrawSegmentedRow("Material Model", material_option, material_options, static_cast<int>(std::size(material_options))))
            render_params.material_model = static_cast<MaterialModel>(material_option);

        EndSection();
    }

    if (BeginSection("Lighting & Shadows"))
    {
        DrawToggleRow("Skybox", &render_params.effect_params.skybox);
        DrawToggleRow("IBL", &render_params.ibl.enable);
        DrawToggleRow("Directional Shadow", &render_params.shadow_params.directional_enable);
        if (render_params.shadow_params.directional_enable)
        {
            static const float scale_options[] = { 0.25f, 0.5f, 1.0f };
            static const char* scale_labels[] = { "25%", "50%", "100%" };
            int scale_option = 1;
            for (int i = 0; i < static_cast<int>(std::size(scale_options)); ++i)
            {
                if (std::abs(render_params.shadow_params.directional_resolution_scale - scale_options[i]) < 0.001f)
                    scale_option = i;
            }

            ImGui::Indent(kChildIndent);
            if (DrawComboRow("Shadow Scale", scale_option, scale_labels, static_cast<int>(std::size(scale_labels))))
            {
                render_params.shadow_params.directional_resolution_scale = scale_options[scale_option];
                g_context.render_system->rebuildRenderTargets();
            }
            ImGui::Unindent(kChildIndent);
        }

        DrawToggleRow("Point Shadow", &render_params.shadow_params.point_enable);
        if (render_params.shadow_params.point_enable)
        {
            static const int point_resolution_options[] = { 512, 1024, 2048 };
            static const char* point_resolution_labels[] = { "512", "1024", "2048" };
            int point_resolution_option = 1;
            for (int i = 0; i < static_cast<int>(std::size(point_resolution_options)); ++i)
            {
                if (render_params.shadow_params.point_cube_resolution == point_resolution_options[i])
                    point_resolution_option = i;
            }

            ImGui::Indent(kChildIndent);
            if (DrawComboRow("Point Shadow Size", point_resolution_option, point_resolution_labels, static_cast<int>(std::size(point_resolution_labels))))
            {
                render_params.shadow_params.point_cube_resolution = point_resolution_options[point_resolution_option];
                g_context.render_system->rebuildRenderTargets();
            }
            ImGui::Unindent(kChildIndent);
        }

        EndSection();
    }

    if (BeginSection("Viewport Effects"))
    {
        DrawToggleRow("Grid", &render_params.effect_params.grid);
        DrawToggleRow("Checkerboard", &render_params.effect_params.checkerboard);
        DrawToggleRow("Wireframe", &render_params.effect_params.wireframe);
        DrawToggleRow("Normals", &render_params.effect_params.show_normal);
        DrawToggleRow("SSAO", &render_params.ssao.enable);
        if (render_params.ssao.enable)
        {
            ImGui::Indent(kChildIndent);
            DrawSliderFloatRow("SSAO Radius", &render_params.ssao.radius, 0.05f, 2.0f, "%.2f");
            DrawSliderFloatRow("SSAO Bias", &render_params.ssao.bias, 0.0f, 0.1f, "%.3f");
            DrawSliderFloatRow("SSAO Power", &render_params.ssao.power, 0.5f, 4.0f, "%.2f");
            ImGui::Unindent(kChildIndent);
        }
        DrawToggleRow("Frustum Culling", &render_params.effect_params.frustum_culling);

        EndSection();
    }

    if (BeginSection("Post Process"))
    {
        DrawToggleRow("FXAA", &render_params.post_processing_params.fxaa);
        DrawToggleRow("Bloom", &render_params.post_processing_params.bloom);
        if (render_params.post_processing_params.bloom)
        {
            ImGui::Indent(kChildIndent);
            DrawSliderFloatRow("Bloom Threshold", &render_params.post_processing_params.bloom_threshold, 0.0f, 5.0f, "%.2f");
            DrawSliderFloatRow("Bloom Soft Knee", &render_params.post_processing_params.bloom_soft_knee, 0.0f, 1.0f, "%.2f");
            DrawSliderFloatRow("Bloom Intensity", &render_params.post_processing_params.bloom_intensity, 0.0f, 5.0f, "%.2f");
            DrawSliderIntRow("Bloom Mip Levels", &render_params.post_processing_params.bloom_mip_levels, 2, 6);
            ImGui::Unindent(kChildIndent);
        }

        DrawToggleRow("Tone Mapping", &render_params.post_processing_params.tone_mapping);
        if (render_params.post_processing_params.tone_mapping)
        {
            ImGui::Indent(kChildIndent);
            DrawSliderFloatRow("Exposure", &render_params.post_processing_params.exposure, 0.1f, 5.0f, "%.2f");
            ImGui::Unindent(kChildIndent);
        }

        EndSection();
    }

    if (BeginSection("Scene Tools"))
    {
        const int point_light_count = g_context.scene ? g_context.scene->pointLightCount() : 0;
        bool minus_pressed = false;
        bool plus_pressed = false;
        DrawStepperRow("Point Lights", point_light_count, minus_pressed, plus_pressed);
        if (minus_pressed)
            deleteLastPointLight();
        if (plus_pressed)
            createPointLight();

        EndSection();
    }

    ImGui::EndChild();
    ImGui::End();
}
