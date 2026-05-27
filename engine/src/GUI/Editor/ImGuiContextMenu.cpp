#include "ImGuiContextMenu.hpp"

#include <imgui.h>

#include "GUI/Editor/ImGuiEditor.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "GlobalContext.hpp"

void ImGuiContextMenu::render()
{
    if (m_menu_opened)
    {
        ImGui::OpenPopup("context_menu");
        m_menu_opened = false;
    }

    if (ImGui::BeginPopup("context_menu"))
    {
        if (m_context == ContextType::Void)
        {
            if (ImGui::BeginMenu("Add"))
            {
                // TODO
                if (ImGui::MenuItem("Add Cube", "", false, true))
                    ;
                if (ImGui::MenuItem("Add Sphere", "", false, true))
                    ;
                ImGui::EndMenu();
            }
        }

        if (m_context == ContextType::Object)
        {
            auto &context_objs = g_context.scene->getPickedObjects();
            if (context_objs.empty())
            {
                ImGui::EndPopup();
                return;
            }
            bool obj_visible = (*context_objs.begin())->visible();
            if (ImGui::MenuItem("Visible", "", obj_visible, true))
            {
                (*context_objs.begin())->setVisible(!obj_visible);
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
