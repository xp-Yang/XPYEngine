#include "ImGuiContextMenu.hpp"

#include <imgui.h>

#include "GUI/Editor/ImGuiEditor.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "Logical/Snapshot/Transaction.hpp"
#include "Logical/Snapshot/UndoRedoStack.hpp"
#include "GlobalContext.hpp"

#include <utility>

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
                auto object = *context_objs.begin();
                Snapshot::Transaction transaction(*g_context.scene, "Toggle Visibility");
                transaction.captureBefore(object->ID());
                object->setVisible(!obj_visible);
                transaction.captureAfter(object->ID());
                if (auto command = transaction.commit())
                    g_context.scene->undoRedoStack().pushExecuted(std::move(command));
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
