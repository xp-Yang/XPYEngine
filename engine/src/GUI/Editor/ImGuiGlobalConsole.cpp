#include "ImGuiGlobalConsole.hpp"

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
#include <utility>
#include <vector>

namespace
{
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
}

void ImGuiGlobalConsole::render() {
    ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoCollapse);

    ImVec2 dummy = ImGui::CalcTextSize("A");
    auto separator = [dummy]() {
        ImGui::Dummy(dummy);
        ImGui::Separator();
        ImGui::Dummy(dummy);
    };

    auto& render_params = g_context.render_system->renderParams();

    ImGui::Dummy(dummy);
    ImGui::PushItemWidth(150.0f);
    int path_type_option = (int)render_params.render_path_type;
    std::array<std::string, 2> combo_strs = { "Forward", "Deferred"/*, "RayTracing"*/ };
    ImGui::Text("Choose Render Path:");
    if (ImGui::BeginCombo("##Render Path", combo_strs[path_type_option].c_str())) {
        for (int i = 0; i < combo_strs.size(); i++) {
            bool selected = path_type_option == i;
            if (ImGui::Selectable(combo_strs[i].c_str(), selected)) {
                render_params.render_path_type = (RenderPathType(i));
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::PushItemWidth(150.0f);
    int resolution_option = (int)render_params.render_resolution;
    std::array<std::string, 3> resolution_labels = { "1080p (1920x1080)", "2K (2560x1440)", "4K (3840x2160)" };
    ImGui::Text("Render Resolution:");
    if (ImGui::BeginCombo("##Render Resolution", resolution_labels[resolution_option].c_str())) {
        for (int i = 0; i < resolution_labels.size(); i++) {
            bool selected = resolution_option == i;
            if (ImGui::Selectable(resolution_labels[i].c_str(), selected)) {
                render_params.render_resolution = (RenderResolutionPreset)i;
                g_context.render_system->rebuildRenderTargets();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    separator();

    ImGui::Text("Material Model:");
    if (ImGui::RadioButton("BlinnPhong", render_params.material_model == MaterialModel::BlinnPhong)) {
        render_params.material_model = MaterialModel::BlinnPhong;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("PBR", render_params.material_model == MaterialModel::PBR)) {
        render_params.material_model = MaterialModel::PBR;
    }

    separator();

    // TODO
    //ImGui::Text("MSAA:");
    //if (render_params.render_path_type == RenderPathType::Forward) {
    //    ImGui::PushItemWidth(50.0f);
    //    static unsigned int curr_item = 1;
    //    if (ImGui::BeginCombo("MSAA", (std::to_string((int)std::pow(4, curr_item)) + "x").c_str())) {
    //        for (int i = 0; i < 3; i++) {
    //            bool selected = curr_item == i;
    //            std::string label = std::to_string((int)std::pow(4, i)) + "x";
    //            if (ImGui::Selectable(label.c_str(), selected)) {
    //                curr_item = i;
    //                render_params.msaa_params.sample_count = (int)std::pow(4, i);
    //            }
    //        }
    //        ImGui::EndCombo();
    //    }
    //    ImGui::SameLine();
    //    static unsigned int shadow_curr_item = 0;
    //    if (ImGui::BeginCombo("Shadow Map Resolution", (std::to_string((int)std::pow(4, shadow_curr_item)) + "x").c_str())) {
    //        for (int i = 0; i < 3; i++) {
    //            bool selected = shadow_curr_item == i;
    //            std::string label = std::to_string((int)std::pow(4, i)) + "x";
    //            if (ImGui::Selectable(label.c_str(), selected)) {
    //                shadow_curr_item = i;
    //                render_params.shadow_params.sample_count = (int)std::pow(4, i);
    //            }
    //        }
    //        ImGui::EndCombo();
    //    }
    //    ImGui::PopItemWidth();
    //    ImGui::SameLine();
    //}
    // 
    //separator();

    ImGui::Text("Effect:");
    ImGui::Checkbox("skybox", &render_params.effect_params.skybox); ImGui::SameLine();
    ImGui::Checkbox("IBL", &render_params.ibl.enable); ImGui::SameLine();
    ImGui::Checkbox("dir shadow", &render_params.shadow_params.directional_enable); ImGui::SameLine();
    ImGui::Checkbox("point shadow", &render_params.shadow_params.point_enable); ImGui::SameLine();
    ImGui::Checkbox("checkerboard", &render_params.effect_params.checkerboard); ImGui::SameLine();
    ImGui::Checkbox("grid", &render_params.effect_params.grid); ImGui::SameLine();
    ImGui::Checkbox("wireframe", &render_params.effect_params.wireframe); ImGui::SameLine();
    ImGui::Checkbox("normal", &render_params.effect_params.show_normal); ImGui::SameLine();
    ImGui::Checkbox("SSAO", &render_params.ssao.enable);
    if (render_params.shadow_params.directional_enable)
    {
        ImGui::PushItemWidth(150.0f);
        static const float directional_scale_options[] = { 0.25f, 0.5f, 1.0f };
        static const char* directional_scale_labels[] = { "25%", "50%", "100%" };
        int directional_scale_idx = 1;
        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(render_params.shadow_params.directional_resolution_scale - directional_scale_options[i]) < 0.001f)
                directional_scale_idx = i;
        }
        ImGui::Text("Directional Shadow Scale:");
        if (ImGui::BeginCombo("##Directional Shadow Scale", directional_scale_labels[directional_scale_idx]))
        {
            for (int i = 0; i < 3; ++i)
            {
                const bool selected = directional_scale_idx == i;
                if (ImGui::Selectable(directional_scale_labels[i], selected))
                {
                    render_params.shadow_params.directional_resolution_scale = directional_scale_options[i];
                    g_context.render_system->rebuildRenderTargets();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
    }
    if (render_params.shadow_params.point_enable)
    {
        ImGui::PushItemWidth(150.0f);
        static const int point_resolution_options[] = { 512, 1024, 2048 };
        int point_resolution_idx = 1;
        for (int i = 0; i < 3; ++i)
        {
            if (render_params.shadow_params.point_cube_resolution == point_resolution_options[i])
                point_resolution_idx = i;
        }
        ImGui::Text("Point Shadow Resolution:");
        if (ImGui::BeginCombo("##Point Shadow Resolution", std::to_string(point_resolution_options[point_resolution_idx]).c_str()))
        {
            for (int i = 0; i < 3; ++i)
            {
                const bool selected = point_resolution_idx == i;
                if (ImGui::Selectable(std::to_string(point_resolution_options[i]).c_str(), selected))
                {
                    render_params.shadow_params.point_cube_resolution = point_resolution_options[i];
                    g_context.render_system->rebuildRenderTargets();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
    }
    if (render_params.ssao.enable)
    {
        ImGui::SliderFloat("ssao radius", &render_params.ssao.radius, 0.05f, 2.0f, "%.2f");
        ImGui::SliderFloat("ssao bias", &render_params.ssao.bias, 0.0f, 0.1f, "%.3f");
        ImGui::SliderFloat("ssao power", &render_params.ssao.power, 0.5f, 4.0f, "%.2f");
    }
    //ImGui::SliderInt("pixel style", &render_params.pixelate_level, 1, 16);
    
    separator();

    ImGui::Text("PostProcess:");
    ImGui::Checkbox("FXAA", &render_params.post_processing_params.fxaa); ImGui::SameLine();
    ImGui::Checkbox("bloom", &render_params.post_processing_params.bloom);
    if (render_params.post_processing_params.bloom)
    {
        ImGui::SliderFloat("bloom threshold", &render_params.post_processing_params.bloom_threshold, 0.0f, 5.0f, "%.2f");
        ImGui::SliderFloat("bloom soft knee", &render_params.post_processing_params.bloom_soft_knee, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("bloom intensity", &render_params.post_processing_params.bloom_intensity, 0.0f, 5.0f, "%.2f");
        ImGui::SliderInt("bloom mip levels", &render_params.post_processing_params.bloom_mip_levels, 2, 6);
    }
    ImGui::Checkbox("tone mapping", &render_params.post_processing_params.tone_mapping);
    if (render_params.post_processing_params.tone_mapping)
    {
        ImGui::SliderFloat("exposure", &render_params.post_processing_params.exposure, 0.1f, 5.0f, "%.2f");
    }

    separator();

    ImGui::Text("Add/Delete point light:");
    int point_light_count = g_context.scene ? g_context.scene->pointLightCount() : 0;
    float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::PushButtonRepeat(true);
    if (ImGui::Button("+##pointLight") && g_context.scene) {
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
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::Button("-##pointLight") && g_context.scene) {
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
    ImGui::PopButtonRepeat();
    ImGui::SameLine();
    ImGui::Text("point light number: %d", point_light_count);

    ImGui::End();
}
