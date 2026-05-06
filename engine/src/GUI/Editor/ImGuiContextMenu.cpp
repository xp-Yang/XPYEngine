#include "ImGuiContextMenu.hpp"

#include <imgui.h>

#include "GUI/Editor/ImGuiEditor.hpp"
#include "Logical/FrameWork/World/Scene.hpp"
#include "GlobalContext.hpp"

void ImGuiContextMenu::render()
{
    if (m_menu_opened) {
        ImGui::OpenPopup("context_menu");
        m_menu_opened = false;
    }

    if (ImGui::BeginPopup("context_menu")) {
        if (m_context == ContextType::Void) {
            if (ImGui::BeginMenu("Add")) {
                if (ImGui::MenuItem("Add Cube", "", false, true))
                    ;
                if (ImGui::MenuItem("Add Sphere", "", false, true))
                    ;
                ImGui::EndMenu();
            }
        }

        if (m_context == ContextType::Object) {
            auto& context_objs = g_context.scene->getPickedObjects();
            bool obj_visible = (*context_objs.begin())->visible();
            if (ImGui::MenuItem("Visible", "", obj_visible, true)) {
                (*context_objs.begin())->setVisible(!obj_visible);
            }

            bool obj_transparent = false;
            auto& sub_meshes = (*context_objs.begin())->getComponent<MeshComponent>()->sub_meshes;
            for (auto& sub_mesh : sub_meshes) {
                if (sub_mesh->material->alpha < 1.0f) {
                    obj_transparent = true;
                    break;
                }
            }
            if (ImGui::MenuItem("Transparent", "", obj_transparent, true)) {
                auto& sub_meshes = (*context_objs.begin())->getComponent<MeshComponent>()->sub_meshes;
                for (auto& sub_mesh : sub_meshes) {
                    sub_mesh->material->alpha = obj_transparent ? 1.f : 0.3f;
                    sub_mesh->material->markDirty();
                }
            }
        }

        if (m_context == ContextType::Light) {
            if (ImGui::MenuItem("Visible", "", false, true)) {
                ;
            }
        }

        ImGui::EndPopup();
    }
}

void ImGuiContextMenu::popUp(ContextType context)
{
    m_menu_opened = true;
    m_context = context;
}
