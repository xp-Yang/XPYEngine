#include "ImGuiGlobalConsole.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "GUI/Editor/ImGuiEditor.hpp"
#include "Render/RenderSystem.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "GlobalContext.hpp"

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
    std::array<std::string, 3> combo_strs = { "Forward", "Deferred", "RayTracing" };
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

    ImGui::Text("MSAA:");
    if (render_params.render_path_type == RenderPathType::Forward) {
        ImGui::PushItemWidth(50.0f);
        static unsigned int curr_item = 1;
        if (ImGui::BeginCombo("MSAA", (std::to_string((int)std::pow(4, curr_item)) + "x").c_str())) {
            for (int i = 0; i < 3; i++) {
                bool selected = curr_item == i;
                std::string label = std::to_string((int)std::pow(4, i)) + "x";
                if (ImGui::Selectable(label.c_str(), selected)) {
                    curr_item = i;
                    render_params.msaa_params.sample_count = (int)std::pow(4, i);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        static unsigned int shadow_curr_item = 0;
        if (ImGui::BeginCombo("Shadow Map Resolution", (std::to_string((int)std::pow(4, shadow_curr_item)) + "x").c_str())) {
            for (int i = 0; i < 3; i++) {
                bool selected = shadow_curr_item == i;
                std::string label = std::to_string((int)std::pow(4, i)) + "x";
                if (ImGui::Selectable(label.c_str(), selected)) {
                    shadow_curr_item = i;
                    render_params.shadow_params.sample_count = (int)std::pow(4, i);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
    }

    separator();

    ImGui::Text("Effect:");
    ImGui::Checkbox("skybox", &render_params.effect_params.skybox); ImGui::SameLine();
    ImGui::Checkbox("shadow", &render_params.shadow_params.enable); ImGui::SameLine();
    ImGui::Checkbox("checkerboard", &render_params.effect_params.checkerboard); ImGui::SameLine();
    ImGui::Checkbox("wireframe", &render_params.effect_params.wireframe); ImGui::SameLine();
    ImGui::Checkbox("normal", &render_params.effect_params.show_normal); ImGui::SameLine();
    //ImGui::SliderInt("pixel style", &render_params.pixelate_level, 1, 16);
    
    separator();

    ImGui::Text("PostProcess:");
    ImGui::Checkbox("FXAA", &render_params.post_processing_params.fxaa); ImGui::SameLine();
    //ImGui::Checkbox("reflection", &render_params.reflection); ImGui::SameLine();
    ImGui::Checkbox("bloom", &render_params.post_processing_params.bloom);

    separator();

    ImGui::Text("Add/Delete point light:");
    auto light_manager = g_context.scene->getLightManager();
    int point_light_count = light_manager ? light_manager->pointLights().size() : 0;
    float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::PushButtonRepeat(true);
    if (ImGui::Button("+##pointLight") && light_manager) {
        light_manager->addPointLight();
    }
    ImGui::SameLine(0.0f, spacing);
    if (ImGui::Button("-##pointLight") && light_manager) {
        light_manager->removeLastPointLight();
    }
    ImGui::PopButtonRepeat();
    ImGui::SameLine();
    ImGui::Text("point light number: %d", point_light_count);

    ImGui::End();
}
