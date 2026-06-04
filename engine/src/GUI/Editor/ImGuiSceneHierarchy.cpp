#include "ImGuiSceneHierarchy.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include "GUI/Editor/ImGuiContextMenu.hpp"
#include "GUI/Editor/ImGuiEditor.hpp"
#include "Logical/Framework/World/SceneDirty.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "Logical/Snapshot/Transaction.hpp"
#include "Logical/Snapshot/UndoRedoStack.hpp"
#include "Render/RenderScene.hpp"
#include "GlobalContext.hpp"

#include <utility>

void ImGuiSceneHierarchy::beginObjectTransaction(GObject* object, const std::string& label)
{
    if (!object || !g_context.scene)
        return;

    const GObjectID id = object->ID();
    if (m_active_transaction && m_active_transaction_object_id != id)
        endObjectTransaction(g_context.scene->objectOf(m_active_transaction_object_id));
    if (m_active_transaction)
        return;

    m_active_transaction = std::make_unique<Snapshot::Transaction>(*g_context.scene, label);
    m_active_transaction_object_id = id;
    m_active_transaction->captureBefore(id);
}

void ImGuiSceneHierarchy::endObjectTransaction(GObject* object)
{
    if (!m_active_transaction || !g_context.scene)
        return;

    const GObjectID id = object ? object->ID() : m_active_transaction_object_id;
    m_active_transaction->captureAfter(id);
    if (auto command = m_active_transaction->commit())
        g_context.scene->undoRedoStack().pushExecuted(std::move(command));

    m_active_transaction.reset();
    m_active_transaction_object_id = {};
}

void ImGuiSceneHierarchy::commitImmediateObjectEdit(GObject* object, const std::string& label, const std::function<void()>& edit)
{
    if (!object || !g_context.scene)
    {
        edit();
        return;
    }

    if (m_active_transaction)
        endObjectTransaction(g_context.scene->objectOf(m_active_transaction_object_id));

    const GObjectID id = object->ID();
    Snapshot::Transaction transaction(*g_context.scene, label);
    transaction.captureBefore(id);
    edit();
    transaction.captureAfter(id);
    if (auto command = transaction.commit())
        g_context.scene->undoRedoStack().pushExecuted(std::move(command));
}

ImGuiSceneHierarchy::~ImGuiSceneHierarchy() = default;

ImGuiSceneHierarchy::ImGuiSceneHierarchy(ImGuiEditor* parent)
    : m_parent(parent)
{
    float columnWidth = 100.0f;

    auto TreeNodeExWithTitleFont = [this](const char* label, ImGuiTreeNodeFlags flags) -> bool
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        const float old_scale = ImGui::GetCurrentWindow()->FontWindowScale;
        ImGui::SetWindowFontScale(1.2f);
        bool node_open = ImGui::TreeNodeEx(label, flags);
        ImGui::SetWindowFontScale(old_scale);
        ImGui::PopStyleColor();
        return node_open;
    };

    auto DrawIntControl = [this, columnWidth](const std::string& label, int& value, float speed = 1.0f, int min = 0.0f, int max = 0.0f, GObject* undo_object = nullptr, const std::string& undo_label = "Edit Property") -> bool {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);

        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();
        int next_value = value;
        bool changed = ImGui::DragInt(("##" + label).c_str(), &next_value, speed, min, max);
        bool should_end_transaction = false;
        if (undo_object)
        {
            if (ImGui::IsItemActivated())
                beginObjectTransaction(undo_object, undo_label);
            if (ImGui::IsItemDeactivatedAfterEdit())
                should_end_transaction = true;
        }
        if (changed)
            value = next_value;
        if (undo_object && should_end_transaction)
            endObjectTransaction(undo_object);

        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
    };

    auto DrawFloatControl = [this, columnWidth](const std::string& label, float& value, float speed = 1.0f, float min = 0.0f, float max = 0.0f, GObject* undo_object = nullptr, const std::string& undo_label = "Edit Property") -> bool {
        ImGui::PushID(label.c_str());

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);

        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();
        float next_value = value;
        bool changed = ImGui::DragFloat(("##" + label).c_str(), &next_value, speed, min, max);
        bool should_end_transaction = false;
        if (undo_object)
        {
            if (ImGui::IsItemActivated())
                beginObjectTransaction(undo_object, undo_label);
            if (ImGui::IsItemDeactivatedAfterEdit())
                should_end_transaction = true;
        }
        if (changed)
            value = next_value;
        if (undo_object && should_end_transaction)
            endObjectTransaction(undo_object);

        ImGui::Columns(1);
        ImGui::PopID();
        return changed;
    };

    enum class VecKind {
        VEC3,
        VEC4,
        COLOR3,
        COLOR4,
    };
    auto DrawVecControl = [this, columnWidth](const std::string& label, const Meta::Instance& inst, const VecKind kind = VecKind::VEC4, float resetValue = 0.0f, GObject* undo_object = nullptr, const std::string& undo_label = "Edit Property") -> bool
    {
        Vec4 values;
        std::string button_label = "X";
        float drag_speed = 0.1f;
        float drag_range_min = 0.0f;
        float drag_range_max = 0.0f;

        bool is_color_kind = (kind == VecKind::COLOR3 || kind == VecKind::COLOR4);
        bool is_dimension_3 = (kind == VecKind::VEC3 || kind == VecKind::COLOR3);
        bool is_dimension_4 = (kind == VecKind::VEC4 || kind == VecKind::COLOR4);

        if (is_dimension_3) {
            values = Vec4(inst.getValue<Vec3>(), 1.0f);
        }
        if (is_dimension_4) {
            values = inst.getValue<Vec4>();
        }

        if (is_color_kind) {
            drag_speed = 0.01f;
            drag_range_min = 0.0f;
            drag_range_max = 1.0f;
        }

        bool changed = false;
        bool should_commit_immediately = false;
        bool should_end_drag_transaction = false;
        ImGui::PushID(label.c_str());

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, ImGui::GetStyle().ItemSpacing.y });

        float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
        if (is_color_kind)
            button_label = "R";
        if (ImGui::Button(button_label.c_str(), buttonSize)) {
            if (undo_object)
                beginObjectTransaction(undo_object, undo_label);
            values.x = resetValue;
            changed = true;
            should_commit_immediately = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        bool axis_changed = ImGui::DragFloat("##btn1", &values.x, drag_speed, drag_range_min, drag_range_max, "%.2f");
        changed |= axis_changed;
        if (undo_object)
        {
            if (ImGui::IsItemActivated())
                beginObjectTransaction(undo_object, undo_label);
            if (ImGui::IsItemDeactivatedAfterEdit())
                should_end_drag_transaction = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.45f, 0.2f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.55f, 0.3f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.45f, 0.2f, 1.0f });
        if (is_color_kind)
            button_label = "G";
        else
            button_label = "Y";
        if (ImGui::Button(button_label.c_str(), buttonSize)) {
            if (undo_object)
                beginObjectTransaction(undo_object, undo_label);
            values.y = resetValue;
            changed = true;
            should_commit_immediately = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        axis_changed = ImGui::DragFloat("##btn2", &values.y, drag_speed, drag_range_min, drag_range_max, "%.2f");
        changed |= axis_changed;
        if (undo_object)
        {
            if (ImGui::IsItemActivated())
                beginObjectTransaction(undo_object, undo_label);
            if (ImGui::IsItemDeactivatedAfterEdit())
                should_end_drag_transaction = true;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
        if (is_color_kind)
            button_label = "B";
        else
            button_label = "Z";
        if (ImGui::Button(button_label.c_str(), buttonSize)) {
            if (undo_object)
                beginObjectTransaction(undo_object, undo_label);
            values.z = resetValue;
            changed = true;
            should_commit_immediately = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        axis_changed = ImGui::DragFloat("##btn3", &values.z, drag_speed, drag_range_min, drag_range_max, "%.2f");
        changed |= axis_changed;
        if (undo_object)
        {
            if (ImGui::IsItemActivated())
                beginObjectTransaction(undo_object, undo_label);
            if (ImGui::IsItemDeactivatedAfterEdit())
                should_end_drag_transaction = true;
        }
        ImGui::PopItemWidth();

        if (is_dimension_4) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.45f, 0.45f, 0.45f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.55f, 0.55f, 0.55f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.45f, 0.45f, 0.45f, 1.0f });
            if (ImGui::Button("W", buttonSize)) {
                if (undo_object)
                    beginObjectTransaction(undo_object, undo_label);
                values.w = resetValue;
                changed = true;
                should_commit_immediately = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            axis_changed = ImGui::DragFloat("##btn4", &values.w, drag_speed, drag_range_min, drag_range_max, "%.2f");
            changed |= axis_changed;
            if (undo_object)
            {
                if (ImGui::IsItemActivated())
                    beginObjectTransaction(undo_object, undo_label);
                if (ImGui::IsItemDeactivatedAfterEdit())
                    should_end_drag_transaction = true;
            }
            ImGui::PopItemWidth();
        }

        ImGui::PopStyleVar();

        ImGui::Columns(1);
        ImGui::PopID();

        if (changed) {
            if (is_dimension_3) {
                inst.getValue<Vec3&>() = values;
            }
            if (is_dimension_4) {
                inst.getValue<Vec4&>() = values;
            }
        }
        if (undo_object && (should_commit_immediately || should_end_drag_transaction))
            endObjectTransaction(undo_object);

        return changed;
    };

    auto DrawTexturePreview = [columnWidth](const std::string& label, const std::shared_ptr<Texture>& tex)
    {
        const float preview_size = 64.f;
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();
        if (!tex) {
            ImGui::TextUnformatted("<none>");
            ImGui::Columns(1);
            return;
        }

        static std::unordered_map<const Texture*, GL_HANDLE> texture_preview_cache;
        auto it = texture_preview_cache.find(tex.get());
        if (it == texture_preview_cache.end()) {
            const GL_HANDLE preview_id = RenderTextureData(tex).id;
            it = texture_preview_cache.insert({ tex.get(), preview_id }).first;
        }

        ImTextureID preview_tex = (ImTextureID)(intptr_t)it->second;
        ImGui::Image(preview_tex, ImVec2(preview_size, preview_size), ImVec2(0, 1), ImVec2(1, 0));
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            if (!tex->texture_filepath.empty())
                ImGui::TextUnformatted(tex->texture_filepath.c_str());
            else
                ImGui::TextUnformatted("<no filepath>");
            ImGui::EndTooltip();
        }
        ImGui::Columns(1);
    };

    m_widget_creator[Meta::MetaTypeOf<Vec4>().typeName()] = [this, DrawVecControl](const std::string& name, const Meta::Instance& inst) -> void
    {
        DrawVecControl(name, inst, VecKind::COLOR4, 1.0f, m_current_inspected_object, "Edit " + name);
    };
    m_widget_creator[Meta::MetaTypeOf<Vec3>().typeName()] = [this, DrawVecControl](const std::string& name, const Meta::Instance& inst) -> void
    {
        DrawVecControl(name, inst, VecKind::VEC3, 0.0f, m_current_inspected_object, "Edit " + name);
    };
    m_widget_creator[Meta::MetaTypeOf<bool>().typeName()] = [this](const std::string& name, const Meta::Instance& inst) -> void
    {
        bool& value = inst.getValue<bool&>();
        bool next_value = value;
        if (ImGui::Checkbox(name.c_str(), &next_value))
        {
            commitImmediateObjectEdit(m_current_inspected_object, "Edit " + name, [&value, next_value]()
            {
                value = next_value;
            });
        }
    };
    m_widget_creator[Meta::MetaTypeOf<int>().typeName()] = [this, DrawIntControl](const std::string& name, const Meta::Instance& inst) -> void
    {
        DrawIntControl(name, inst.getValue<int&>(), 1.0f, 0, 0, m_current_inspected_object, "Edit " + name);
    };
    m_widget_creator[Meta::MetaTypeOf<float>().typeName()] = [this, DrawFloatControl](const std::string& name, const Meta::Instance& inst) -> void
    {
        DrawFloatControl(name, inst.getValue<float&>(), 1.0f, 0.0f, 0.0f, m_current_inspected_object, "Edit " + name);
    };
    m_widget_creator[Meta::MetaTypeOf<std::string>().typeName()] = [this, columnWidth](const std::string& name, const Meta::Instance& inst) -> void
    {
        ImGui::PushID(name.c_str());

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, columnWidth);

        ImGui::Text(name.c_str());
        ImGui::NextColumn();
        std::string& val = inst.getValue<std::string&>();
        if (val.empty())
            ImGui::Text("<none>");
        else
            ImGui::Text(val.c_str());

        ImGui::Columns(1);
        ImGui::PopID();
    };
    m_widget_creator[Meta::MetaTypeOf<CameraComponent>().typeName()] = [this, DrawFloatControl, DrawVecControl, TreeNodeExWithTitleFont](const std::string& name, const Meta::Instance& inst) -> void
    {
        auto& camera = inst.getValue<CameraComponent&>();
        GObject* owner = camera.parent_object;
        bool node_open = TreeNodeExWithTitleFont(inst.typeName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
        if (node_open)
        {
            bool view_changed = false;
            bool projection_changed = false;
            SceneDirtyFlags dirty_flags = SceneDirtyFlagBit(SceneDirtyFlag::None);

            // 这里编辑的是 CameraComponent 的运行时相机状态。
            // TransformComponent 只负责场景对象位置，所以 pos 被修改时要同步回 Transform。
            view_changed |= DrawVecControl("pos", camera.pos, VecKind::VEC3, 0.0f, owner, "Edit Camera Position");
            view_changed |= DrawVecControl("direction", camera.direction, VecKind::VEC3, 0.0f, owner, "Edit Camera Direction");
            view_changed |= DrawVecControl("upDirection", camera.upDirection, VecKind::VEC3, 0.0f, owner, "Edit Camera Up Direction");
            projection_changed |= DrawFloatControl("fov", camera.fov, 0.01f, Math::deg2rad(0.01f), Math::deg2rad(135.0f), owner, "Edit Camera FOV");
            projection_changed |= DrawFloatControl("nearPlane", camera.nearPlane, 0.01f, 0.001f, camera.farPlane, owner, "Edit Camera Near Plane");
            projection_changed |= DrawFloatControl("farPlane", camera.farPlane, 1.0f, camera.nearPlane, 10000.0f, owner, "Edit Camera Far Plane");
            projection_changed |= DrawFloatControl("aspectRatio", camera.aspectRatio, 0.01f, 0.1f, 10.0f, owner, "Edit Camera Aspect Ratio");

            if (view_changed)
            {
                camera.refreshView();
                if (camera.parent_object)
                {
                    if (auto* transform = camera.parent_object->getComponent<TransformComponent>())
                        transform->translation = camera.pos;
                    dirty_flags |= SceneDirtyFlagBit(SceneDirtyFlag::Transform);
                }
            }
            if (projection_changed)
                camera.refreshProjection();
            if (view_changed || projection_changed)
            {
                dirty_flags |= SceneDirtyFlagBit(SceneDirtyFlag::Camera);
                if (camera.parent_object)
                    camera.parent_object->markDirty(dirty_flags);
            }

            ImGui::TreePop();
        }
    };
    m_widget_creator[Meta::MetaTypeOf<TransformComponent>().typeName()] = [this, DrawVecControl, TreeNodeExWithTitleFont](const std::string& name, const Meta::Instance& inst) -> void
    {
        auto& transform = inst.getValue<TransformComponent&>();
        GObject* owner = transform.parent_object;
        bool node_open = false;
        node_open = TreeNodeExWithTitleFont(inst.typeName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
        if (node_open)
        {
            bool transform_changed = false;
            Meta::MetaType meta = inst.metaType();
            for (auto& prop : meta.properties())
            {
                if (prop.isType<Vec3>()) {
                    if (prop.name == "scale")
                        transform_changed |= DrawVecControl(prop.name, prop.getValue(inst), VecKind::VEC3, 1.0f, owner, "Edit Transform " + prop.name);
                    else
                        transform_changed |= DrawVecControl(prop.name, prop.getValue(inst), VecKind::VEC3, 0.0f, owner, "Edit Transform " + prop.name);
                }
            }
            if (transform_changed)
            {
                if (transform.parent_object)
                {
                    SceneDirtyFlags dirty_flags = SceneDirtyFlagBit(SceneDirtyFlag::Transform);
                    if (auto* camera = transform.parent_object->getComponent<CameraComponent>())
                    {
                        // 在 Inspector 里直接改 Main Camera 的 Transform 时，
                        // 也要把位置同步给 CameraComponent，否则渲染相机会停在旧位置。
                        camera->pos = transform.translation;
                        camera->refreshView();
                        dirty_flags |= SceneDirtyFlagBit(SceneDirtyFlag::Camera);
                    }
                    transform.parent_object->markDirty(dirty_flags);
                }
            }
            ImGui::TreePop();
        }
    };
    m_widget_creator[Meta::MetaTypeOf<MeshComponent>().typeName()] = [this, DrawFloatControl, DrawVecControl, DrawTexturePreview, columnWidth, TreeNodeExWithTitleFont](const std::string& name, const Meta::Instance& inst) -> void
    {
        auto DrawSubMeshControl = [DrawTexturePreview, DrawFloatControl, DrawVecControl, columnWidth, TreeNodeExWithTitleFont](const std::string& label, Mesh& sub_mesh, GObject* owner) -> SceneDirtyFlags
        {
            SceneDirtyFlags dirty_flags = SceneDirtyFlagBit(SceneDirtyFlag::None);
            ImGui::PushID(label.c_str());
            if (TreeNodeExWithTitleFont(label.c_str(), ImGuiTreeNodeFlags_SpanFullWidth)) {
                ImGui::Columns(2, nullptr, false);
                ImGui::SetColumnWidth(0, columnWidth);
                ImGui::Text("vertices");
                ImGui::NextColumn();
                ImGui::Text("%d", sub_mesh.geometry ? (int)sub_mesh.geometry->vertices.size() : 0);
                ImGui::NextColumn();
                ImGui::Text("indices");
                ImGui::NextColumn();
                ImGui::Text("%d", sub_mesh.geometry ? (int)sub_mesh.geometry->indices.size() : 0);
                ImGui::Columns(1);

                if (TreeNodeExWithTitleFont(("Local Transform##" + label).c_str(), ImGuiTreeNodeFlags_SpanFullWidth))
                {
                    bool local_transform_changed = false;
                    local_transform_changed |= DrawVecControl("translation", sub_mesh.translation, VecKind::VEC3, 0.0f, owner, "Edit Mesh Translation");
                    local_transform_changed |= DrawVecControl("rotation", sub_mesh.rotation, VecKind::VEC3, 0.0f, owner, "Edit Mesh Rotation");
                    local_transform_changed |= DrawVecControl("scale", sub_mesh.scale, VecKind::VEC3, 1.0f, owner, "Edit Mesh Scale");
                    if (local_transform_changed)
                        dirty_flags |= SceneDirtyFlagBit(SceneDirtyFlag::Mesh);
                    ImGui::TreePop();
                }

                if (sub_mesh.material) {
                    if (TreeNodeExWithTitleFont(("Material##" + label).c_str(), ImGuiTreeNodeFlags_SpanFullWidth))
                    {
                        bool material_changed = false;
                        material_changed |= DrawVecControl("base_color_factor", sub_mesh.material->base_color_factor, VecKind::COLOR3, 1.0f, owner, "Edit Material Base Color");
                        material_changed |= DrawFloatControl("metallic_factor", sub_mesh.material->metallic_factor, 0.01f, 0.0f, 1.0f, owner, "Edit Material Metallic");
                        material_changed |= DrawFloatControl("roughness_factor", sub_mesh.material->roughness_factor, 0.01f, 0.0f, 1.0f, owner, "Edit Material Roughness");
                        material_changed |= DrawFloatControl("ao_factor", sub_mesh.material->ao_factor, 0.01f, 0.0f, 1.0f, owner, "Edit Material AO");
                        material_changed |= DrawVecControl("diffuse_factor", sub_mesh.material->diffuse_factor, VecKind::COLOR3, 1.0f, owner, "Edit Material Diffuse");
                        material_changed |= DrawVecControl("specular_factor", sub_mesh.material->specular_factor, VecKind::COLOR3, 1.0f, owner, "Edit Material Specular");
                        material_changed |= DrawFloatControl("shininess", sub_mesh.material->shininess, 1.0f, 1.0f, 1024.0f, owner, "Edit Material Shininess");
                        material_changed |= DrawFloatControl("alpha", sub_mesh.material->alpha, 0.01f, 0.0f, 1.0f, owner, "Edit Material Alpha");
                        if (material_changed)
                        {
                            sub_mesh.material->markDirty();
                            dirty_flags |= SceneDirtyFlagBit(SceneDirtyFlag::Material);
                        }

                        DrawTexturePreview("albedo", sub_mesh.material->albedo_texture);
                        DrawTexturePreview("metallic", sub_mesh.material->metallic_texture);
                        DrawTexturePreview("roughness", sub_mesh.material->roughness_texture);
                        DrawTexturePreview("ao", sub_mesh.material->ao_texture);

                        DrawTexturePreview("diffuse", sub_mesh.material->diffuse_texture);
                        DrawTexturePreview("specular", sub_mesh.material->specular_texture);
                        DrawTexturePreview("normal", sub_mesh.material->normal_texture);
                        DrawTexturePreview("height", sub_mesh.material->height_texture);

                        ImGui::TreePop();
                    }
                }
                else {
                    ImGui::TextUnformatted("Material: <none>");
                }

                ImGui::TreePop();
            }
            ImGui::PopID();
            return dirty_flags;
        };

        static ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings;
        bool node_open = false;
        node_open = TreeNodeExWithTitleFont(inst.typeName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
        if (node_open)
        {
            for (auto& prop : inst.metaType().properties())
            {
                MeshComponent& mc = inst.getValue<MeshComponent&>();
                if (prop.name == "staticShadowCaster") {
                    bool next_value = mc.staticShadowCaster;
                    if (ImGui::Checkbox("staticShadowCaster", &next_value))
                    {
                        commitImmediateObjectEdit(mc.parent_object, "Edit Static Shadow Caster", [&mc, next_value]()
                        {
                            mc.staticShadowCaster = next_value;
                        });
                        if (mc.parent_object)
                            mc.parent_object->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Mesh));
                    }
                }
                else if (prop.name == "sub_meshes") {
                    SceneDirtyFlags dirty_flags = SceneDirtyFlagBit(SceneDirtyFlag::None);
                    for (auto& sub_mesh : mc.sub_meshes)
                    {
                        dirty_flags |= DrawSubMeshControl(std::string("SubMesh id ") + std::to_string(sub_mesh->sub_mesh_idx), *sub_mesh, mc.parent_object);
                    }
                    if (dirty_flags != SceneDirtyFlagBit(SceneDirtyFlag::None) && mc.parent_object)
                        mc.parent_object->markDirty(dirty_flags);
                }
                else if (m_widget_creator.find(prop.type_name) != m_widget_creator.end())
                    m_widget_creator[prop.type_name](prop.name, prop.getValue(inst));
            }

            ImGui::TreePop();
        }
    };
    m_widget_creator[Meta::MetaTypeOf<AnimationComponent>().typeName()] = [this, DrawFloatControl, TreeNodeExWithTitleFont](const std::string& name, const Meta::Instance& inst) -> void
    {
        auto& animation = inst.getValue<AnimationComponent&>();
        GObject* owner = animation.parent_object;
        bool node_open = TreeNodeExWithTitleFont(inst.typeName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
        if (node_open)
        {
            for (auto& prop : inst.metaType().properties())
            {
                if (prop.name == "speed") {
                    DrawFloatControl(prop.name, prop.getValue(inst).getValue<float&>(), 0.01f, 0.0f, 2.0f, owner, "Edit Animation Speed");
                }
                else if (m_widget_creator.find(prop.type_name) != m_widget_creator.end())
                    m_widget_creator[prop.type_name](prop.name, prop.getValue(inst));
            }

            ImGui::TreePop();
        }
    };
    m_widget_creator[Meta::MetaTypeOf<PointLightComponent>().typeName()] = [this, DrawFloatControl, DrawVecControl, TreeNodeExWithTitleFont](const std::string& name, const Meta::Instance& inst) -> void
    {
        auto& light = inst.getValue<PointLightComponent&>();
        GObject* owner = light.parent_object;
        bool node_open = TreeNodeExWithTitleFont(inst.typeName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
        if (node_open)
        {
            bool light_changed = false;
            light_changed |= DrawVecControl("luminousColor", light.luminousColor, VecKind::COLOR3, 1.0f, owner, "Edit Point Light Color");
            light_changed |= DrawFloatControl("radius", light.radius, 0.1f, 0.01f, 1000.0f, owner, "Edit Point Light Radius");
            bool next_cast_shadow = light.castShadow;
            if (ImGui::Checkbox("castShadow", &next_cast_shadow))
            {
                commitImmediateObjectEdit(owner, "Edit Point Light Shadow", [&light, next_cast_shadow]()
                {
                    light.castShadow = next_cast_shadow;
                });
                light_changed = true;
            }
            if (light_changed && light.parent_object)
                light.parent_object->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Light));
            ImGui::TreePop();
        }
    };
    m_widget_creator[Meta::MetaTypeOf<DirectionalLightComponent>().typeName()] = [this, DrawFloatControl, DrawVecControl, TreeNodeExWithTitleFont](const std::string& name, const Meta::Instance& inst) -> void
    {
        auto& light = inst.getValue<DirectionalLightComponent&>();
        GObject* owner = light.parent_object;
        bool node_open = TreeNodeExWithTitleFont(inst.typeName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth);
        if (node_open)
        {
            bool light_changed = false;
            light_changed |= DrawVecControl("luminousColor", light.luminousColor, VecKind::COLOR3, 1.0f, owner, "Edit Directional Light Color");
            light_changed |= DrawVecControl("direction", light.direction, VecKind::VEC3, 0.0f, owner, "Edit Directional Light Direction");
            light_changed |= DrawFloatControl("aspectRatio", light.aspectRatio, 0.01f, 0.1f, 10.0f, owner, "Edit Directional Light Aspect Ratio");
            if (light_changed && light.parent_object)
                light.parent_object->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Light));
            ImGui::TreePop();
        }
    };
    m_widget_creator[Meta::MetaTypeOf<GObject>().typeName()] = [this, TreeNodeExWithTitleFont](const std::string& name, const Meta::Instance& inst) -> void
    {
        auto& object = inst.getValue<GObject&>();
        GObjectID id = object.ID();
        std::string display_text = name + " (ID: " + std::to_string(id.value()) + ")";

        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        const auto& original_picked_ids = g_context.scene->getPickedObjectIDs();
        if (std::find(original_picked_ids.begin(), original_picked_ids.end(), id) != original_picked_ids.end())
            node_flags |= ImGuiTreeNodeFlags_Selected;
        else
            node_flags &= ~ImGuiTreeNodeFlags_Selected;

        bool node_open = TreeNodeExWithTitleFont(display_text.c_str(), node_flags);
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            g_context.scene->onPickedChanged({ id }, original_picked_ids);
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            g_context.scene->onPickedChanged({ id }, original_picked_ids);
            m_parent->popUpMenu();
        }
        if (node_open)
        {
            GObject* previous_inspected_object = m_current_inspected_object;
            m_current_inspected_object = &object;
            if (object.isLeaf())
            {
                for (auto& com : object.getComponents())
                {
                    Meta::Instance inst{ *com };
                    if (m_widget_creator.find(inst.typeName()) != m_widget_creator.end())
                        m_widget_creator[inst.typeName()](inst.typeName(), inst);
                }
            }
            else
            {
                for (auto& child : object.children())
                {
                    Meta::Instance inst{ *child };
                    m_widget_creator[Meta::MetaTypeOf<GObject>().typeName()](inst.typeName(), inst);
                }
            }
            m_current_inspected_object = previous_inspected_object;
            ImGui::TreePop();
        }
    };
}

void ImGuiSceneHierarchy::render()
{
    if (ImGui::Begin(("Scene Hierarchy"), nullptr, ImGuiWindowFlags_NoCollapse))
    {
        ImGuiWindow *scene_hierarchy_window = ImGui::GetCurrentWindow();

        const std::vector<std::shared_ptr<GObject>> &objects = g_context.scene->getObjects();
        for (int i = 0; i < objects.size(); i++)
        {
            auto object = objects[i];
            std::string child_name = object->name();
            Meta::Instance inst{*object};
            if (m_widget_creator.find(Meta::MetaTypeOf<GObject>().typeName()) != m_widget_creator.end())
                m_widget_creator[Meta::MetaTypeOf<GObject>().typeName()](child_name, inst);
        }
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered())
        {
            g_context.scene->onPickedChanged({}, g_context.scene->getPickedObjectIDs());
            m_parent->popUpMenu();
        }
    }
    ImGui::End();
}

#if ENABLE_ECS
void ImGuiEditor::renderPickedEntityController(const ImVec2 &pos, const std::vector<ecs::Entity> &picked_entities)
{
    if (picked_entities.empty())
        return;
    auto entity = picked_entities[0];
    if (!world.hasComponent<ecs::NameComponent>(entity))
        return;
    std::string obj_name = world.getComponent<ecs::NameComponent>(entity)->name;
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::Begin((obj_name).c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

    if (world.hasComponent<ecs::RenderableComponent>(entity))
    {
        auto renderable = world.getComponent<ecs::RenderableComponent>(entity);
        for (int i = 0; i < renderable->sub_meshes.size(); i++)
        {
            auto &material = renderable->sub_meshes[i].material;

            ImGui::PushItemWidth(80.0f);
            ImGui::SliderFloat((std::string("##albedo.x") + "##" + obj_name).c_str(), &material.albedo.x, 0.0f, 1.0f);
            ImGui::SameLine();
            ImGui::SliderFloat((std::string("##albedo.y") + "##" + obj_name).c_str(), &material.albedo.y, 0.0f, 1.0f);
            ImGui::SameLine();
            ImGui::SliderFloat((std::string("albedo") + "##" + obj_name).c_str(), &material.albedo.z, 0.0f, 1.0f);
            ImGui::PopItemWidth();

            ImGui::SliderFloat((std::string("metallic") + "##" + obj_name).c_str(), &material.metallic, 0.0f, 1.0f);
            ImGui::SliderFloat((std::string("roughness") + "##" + obj_name).c_str(), &material.roughness, 0.01f, 1.0f);
            ImGui::SliderFloat((std::string("ao") + "##" + obj_name).c_str(), &material.ao, 0.0f, 1.0f);
        }
    }
    if (world.hasComponent<ecs::ExplosionComponent>(entity))
    {
        auto &explosion = *world.getComponent<ecs::ExplosionComponent>(entity);
        ImGui::SliderFloat((std::string("explosion ratio") + "##" + obj_name).c_str(), &explosion.explosionRatio, 0.0f, 1.0f);
    }
    if (world.hasComponent<ecs::PointLightComponent>(entity))
    {
        Vec4 &luminousColor = world.getComponent<ecs::PointLightComponent>(entity)->luminousColor;
        float *radius = &world.getComponent<ecs::PointLightComponent>(entity)->radius;
        ImGui::ColorEdit3((std::string("Luminous Color") + "##" + obj_name).c_str(), (float *)&luminousColor);
        ImGui::SliderFloat((std::string("Radius") + "##" + obj_name).c_str(), radius, 5.0f, 50.0f);
    }
    if (world.hasComponent<ecs::DirectionalLightComponent>(entity))
    {
        auto dir_light_component = world.getComponent<ecs::DirectionalLightComponent>(entity);
        Vec4 &luminousColor = dir_light_component->luminousColor;
        ImGui::ColorEdit3((std::string("Luminous Color") + "##" + obj_name).c_str(), (float *)&luminousColor);
    }

    ImGui::End();
}
#endif // ENABLE_ECS
