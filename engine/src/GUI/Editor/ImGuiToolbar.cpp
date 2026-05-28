#include "ImGuiToolbar.hpp"
#include "ImGuiCanvas.hpp"
#include "AssetManager/Texture.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "Render/RenderSourceData.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

ImGuiToolbar::ImGuiToolbar(ImGuiCanvas *parent, std::shared_ptr<Scene> scene)
    : m_parent(parent), ref_scene(scene)
{
    auto tranlate_icon = std::make_shared<Texture>(TextureType::Custom, std::string(ASSET_DIR) + "/images/toolbar_translation.png", false);
    auto rotate_icon = std::make_shared<Texture>(TextureType::Custom, std::string(ASSET_DIR) + "/images/toolbar_rotate.png", false);
    auto scale_icon = std::make_shared<Texture>(TextureType::Custom, std::string(ASSET_DIR) + "/images/toolbar_scale.png", false);

    m_tranlate_icon_id = RenderTextureData(tranlate_icon).id;
    m_rotate_icon_id = RenderTextureData(rotate_icon).id;
    m_scale_icon_id = RenderTextureData(scale_icon).id;
}

void ImGuiToolbar::render()
{
    renderToolbar();
    renderGizmos();
}

void ImGuiToolbar::renderToolbar()
{
    auto main_canvas_rect = m_parent->canvasRect();
    ImGuiStyle &style = ImGui::GetStyle();
    ImVec2 button_size = ImVec2(40, 40);
    ImVec2 toolbar_size = ImVec2(3 * button_size.x + 4 * style.ItemSpacing.x, button_size.y + 2 * style.ItemSpacing.x);
    ImVec2 toolbar_pos = ImVec2(main_canvas_rect.width / 2.f - toolbar_size.x / 2.0f, 0.0f);

    ImVec2 toolbar_screen_pos = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin() + toolbar_pos;
    ImGui::RenderFrame(toolbar_screen_pos,
                       toolbar_screen_pos + toolbar_size,
                       ImColor(0.15f, 0.15f, 0.15f, 1.0f), false, 4.0f);

    ImGui::SetCursorPos(ImGui::GetWindowContentRegionMin() + toolbar_pos + ImVec2(style.ItemSpacing.x, style.ItemSpacing.x));
    ImVec4 tintColor = m_toolbar_type == ToolbarType::Translate ? ImVec4(1, 1, 1, 1) : ImVec4(0.5, 0.5, 0.5, 1);
    if (ImGui::ImageButton(ImTextureID(m_tranlate_icon_id), button_size, ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0), tintColor))
        m_toolbar_type = ToolbarType::Translate;
    ImGui::SameLine();
    tintColor = m_toolbar_type == ToolbarType::Rotate ? ImVec4(1, 1, 1, 1) : ImVec4(0.5, 0.5, 0.5, 1);
    if (ImGui::ImageButton(ImTextureID(m_rotate_icon_id), button_size, ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0), tintColor))
        m_toolbar_type = ToolbarType::Rotate;
    ImGui::SameLine();
    tintColor = m_toolbar_type == ToolbarType::Scale ? ImVec4(1, 1, 1, 1) : ImVec4(0.5, 0.5, 0.5, 1);
    if (ImGui::ImageButton(ImTextureID(m_scale_icon_id), button_size, ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0), tintColor))
        m_toolbar_type = ToolbarType::Scale;
}

void ImGuiToolbar::renderGizmos()
{
    ImGuizmo::OPERATION imguizmo_operation;
    switch (m_toolbar_type)
    {
    case ToolbarType::Translate:
        imguizmo_operation = ImGuizmo::TRANSLATE;
        break;
    case ToolbarType::Rotate:
        imguizmo_operation = ImGuizmo::ROTATE;
        break;
    case ToolbarType::Scale:
        imguizmo_operation = ImGuizmo::SCALE;
        break;
    default:
        break;
    }

    auto main_canvas_rect = m_parent->canvasRect();

    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(main_canvas_rect.x, main_canvas_rect.y, main_canvas_rect.width, main_canvas_rect.height);

    auto &camera = ref_scene->getMainCamera();

    for (const auto &object : ref_scene->getPickedObjects())
    {
        TransformComponent *transform_component = object->getComponent<TransformComponent>();
        if (!transform_component)
            continue;
        Mat4 model_matrix = transform_component->transform();
        ImGuizmo::Manipulate((float *)(&camera.view), (float *)(&camera.projection), imguizmo_operation, ImGuizmo::LOCAL, (float *)(&model_matrix), NULL, NULL, NULL, NULL);
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents((float *)&model_matrix, matrixTranslation, matrixRotation, matrixScale);
        transform_component->translation = Vec3(matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]);
        transform_component->scale = Vec3(matrixScale[0], matrixScale[1], matrixScale[2]);
        transform_component->rotation = Vec3(matrixRotation[0], matrixRotation[1], matrixRotation[2]);

        if (auto* camera_component = object->getComponent<CameraComponent>())
        {
            // Main Camera 现在也是普通 GObject。Gizmo 操作改的是 Transform，
            // 渲染读的是 CameraComponent，因此这里把位置同步回相机状态并刷新 view。
            // 旋转同步后续可以再做成完整的 Transform -> camera direction/up 推导。
            camera_component->pos = transform_component->translation;
            camera_component->refreshView();
        }
    }

    ImVec2 air_window_size = ImVec2(128, 128);
    ImVec2 air_window_pos = ImVec2(main_canvas_rect.x, main_canvas_rect.y);
    float camDistance = 8.f;
    ImGuizmo::ViewManipulate((float *)(&camera.view), camDistance, air_window_pos, air_window_size, 0x10101010);

#if ENABLE_ECS
    ecs::TransformComponent *transform_component = nullptr;
    auto picked_entities = world.getPickedEntities();
    for (auto entity : picked_entities)
    {
        transform_component = world.getComponent<ecs::TransformComponent>(entity);
        break;
    }

    if (transform_component)
    {
        Mat4 model_matrix = transform_component->transform();
        ImGuizmo::Manipulate((float *)(&camera.view), (float *)(&camera.projection), imguizmo_operation, ImGuizmo::LOCAL, (float *)(&model_matrix), NULL, NULL, NULL, NULL);
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents((float *)&model_matrix, matrixTranslation, matrixRotation, matrixScale);
        transform_component->translation = Vec3(matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]);
        transform_component->scale = Vec3(matrixScale[0], matrixScale[1], matrixScale[2]);
        transform_component->rotation = Vec3(matrixRotation[0], matrixRotation[1], matrixRotation[2]);
    }

    if (camera)
    {
        IntRect canvas_rect = m_parent->canvasRect();
        ImVec2 air_window_size = ImVec2(128, 128);
        float camDistance = 8.f;
        ImGuizmo::ViewManipulate((float *)(&camera->view), camDistance, ImVec2(canvas_rect.x + canvas_rect.width - air_window_size.x, canvas_rect.y), air_window_size, 0x10101010);
    }
#endif
}
