#include "ImGuiContextMenu.hpp"

#include <imgui.h>

#include "GUI/Editor/ImGuiEditor.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "Logical/Snapshot/ObjectSnapshotCommand.hpp"
#include "Logical/Snapshot/ObjectSnapshotService.hpp"
#include "Logical/Snapshot/Transaction.hpp"
#include "Logical/Snapshot/UndoRedoStack.hpp"
#include "GlobalContext.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    void pushCreateObjectCommand(Scene& scene, GObject& object, const std::string& label)
    {
        Snapshot::ObjectSnapshot before;
        before.id = object.ID();
        before.existed = false;

        Snapshot::ObjectSnapshot after = Snapshot::ObjectSnapshotService::capture(scene, object.ID());
        if (!after.existed)
            return;

        scene.undoRedoStack().pushExecuted(
            std::make_unique<Snapshot::ObjectSnapshotCommand>(
                label,
                std::vector<Snapshot::ObjectSnapshot>{ before },
                std::vector<Snapshot::ObjectSnapshot>{ std::move(after) }));
    }

    void addBasicModelObject(const std::string& name, const std::string& model_relative_path, const std::string& command_label)
    {
        if (!g_context.scene)
            return;

        GObject* object = g_context.scene->loadModel(std::string(ASSET_DIR) + model_relative_path);
        if (!object)
            return;

        object->setName(name);
        pushCreateObjectCommand(*g_context.scene, *object, command_label);
    }
}

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
                if (ImGui::MenuItem("Add Cube", "", false, true))
                    addBasicModelObject("Cube", "/model/basic/cube.obj", "Add Cube");
                if (ImGui::MenuItem("Add Sphere", "", false, true))
                    addBasicModelObject("Sphere", "/model/basic/sphere.obj", "Add Sphere");
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
