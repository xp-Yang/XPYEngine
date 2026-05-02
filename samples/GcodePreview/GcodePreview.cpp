#include "GcodePreview.hpp"
#include "GcodeImporter/GcodeImporter.hpp"
#include "Base/Logger/Logger.hpp"
#include "Base/Signal/Signal.hpp"
#include <imgui.h>

static std::shared_ptr<Texture> makeRoleColorTexture(const Color4& color)
{
    auto texture = std::make_shared<Texture>(TextureType::Custom, std::string(RESOURCE_DIR) + "/images/pure_white_map.png", false);
    if (!texture || !texture->data || texture->channel_count < 3 || texture->width <= 0 || texture->height <= 0) {
        return texture;
    }

    const unsigned char r = static_cast<unsigned char>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
    const unsigned char g = static_cast<unsigned char>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
    const unsigned char b = static_cast<unsigned char>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
    const unsigned char a = static_cast<unsigned char>(std::clamp(color.a, 0.0f, 1.0f) * 255.0f);

    const int pixel_count = texture->width * texture->height;
    for (int i = 0; i < pixel_count; ++i) {
        const int idx = i * texture->channel_count;
        texture->data[idx + 0] = r;
        texture->data[idx + 1] = g;
        texture->data[idx + 2] = b;
        if (texture->channel_count >= 4) {
            texture->data[idx + 3] = a;
        }
    }
    return texture;
}

GcodePreview::GcodePreview()
    : m_gcode_trace(std::make_unique<GcodeTrace>())
{
    connect(
        m_gcode_trace.get(),
        &(m_gcode_trace->loaded),
        std::function<void(std::array<LinesBatch, ExtrusionRole::erCount>)>(
            [this](std::array<LinesBatch, ExtrusionRole::erCount> lines_batches) {
                buildMesh(lines_batches);
            }));
}

bool GcodePreview::loadFromFile(const std::string& filepath, const std::shared_ptr<Scene>& scene)
{
    if (!scene) {
        return false;
    }
    GCodeProcessor gcode_processor;
    gcode_processor.process_file(filepath);
    m_result = gcode_processor.extract_result();
    m_source_filepath = filepath;
    m_scene = scene;

    m_gcode_trace->load(m_result);

    m_loaded = true;
    return true;
}

void GcodePreview::buildMesh(const std::array<LinesBatch, ExtrusionRole::erCount>& lines_batches)
{
    if (!m_gcodes_object) {
        GObject* raw_object = GObject::create(nullptr, "GcodePreview");
        raw_object->addComponent<TransformComponent>();
        raw_object->addComponent<MeshComponent>();
        m_gcodes_object = std::shared_ptr<GObject>(raw_object);
        m_scene->addObject(m_gcodes_object);
        // Gcode axis (X,Y,Z) maps to engine axis (X,-Z,Y), which is -90 deg around X.
        if (auto transform = m_gcodes_object->getComponent<TransformComponent>()) {
            transform->translation = Vec3(-128.0f * 50.0f / 256.0f, 0.0f, 128.0f * 50.0f / 256.0f);
            transform->rotation = Vec3(-90.0f, 0.0f, 0.0f);
            transform->scale = Vec3(50.0f / 256.0f);
        }
    }

    auto mesh_component = m_gcodes_object->getComponent<MeshComponent>();
    mesh_component->sub_meshes.clear();

    for (int i = 0; i < static_cast<int>(lines_batches.size()); i++) {
        if (lines_batches[i].empty()) {
            continue;
        }

        std::shared_ptr<SimpleMesh> simple_mesh = lines_batches[i].merged_mesh;
        std::vector<SimpleVertex>& simple_vertices = simple_mesh->vertices;
        std::vector<Vertex> vertices;
        vertices.resize(simple_vertices.size());
        for (int j = 0; j < static_cast<int>(simple_vertices.size()); j++) {
            auto& simple_vertex = simple_vertices[j];
            vertices[j] = { simple_vertex.position, simple_vertex.normal, Vec2(0.0f) };
        }

        std::shared_ptr<Mesh> colored_mesh = std::make_shared<Mesh>(vertices, simple_mesh->indices, std::make_shared<Material>());
        colored_mesh->sub_mesh_idx = i * 2;
        const auto color_texture = makeRoleColorTexture(Extrusion_Role_Colors[i]);
        colored_mesh->material->diffuse_texture = color_texture;
        colored_mesh->material->albedo_texture = color_texture;
        mesh_component->sub_meshes.push_back(colored_mesh);

        std::shared_ptr<Mesh> colorless_mesh = std::make_shared<Mesh>(vertices, simple_mesh->indices, std::make_shared<Material>());
        colorless_mesh->sub_mesh_idx = i * 2 + 1;
        const auto colorless_texture = makeRoleColorTexture(Silent_Color);
        colorless_mesh->material->diffuse_texture = color_texture;
        colorless_mesh->material->albedo_texture = color_texture;
        mesh_component->sub_meshes.push_back(colorless_mesh);
    }
}

void GcodePreview::rebuildMeshRange(const std::array<LinesBatch, ExtrusionRole::erCount>& lines_batches)
{
    if (!m_gcodes_object) {
        return;
    }

    auto mesh_component = m_gcodes_object->getComponent<MeshComponent>();
    auto& sub_meshes = mesh_component->sub_meshes;

    for (int i = 0; i < static_cast<int>(sub_meshes.size()); i++) {
        auto& mesh = sub_meshes[i];
        bool is_colored = (i % 2 == 0);
        int role_idx = mesh->sub_mesh_idx / 2;

        if (lines_batches[role_idx].empty()) {
            mesh->index_offset = 0;
            mesh->index_count = 0;
            continue;
        }

        const auto& index_offset = is_colored ?
            m_gcode_trace->colored_indices_interval(ExtrusionRole(role_idx)) :
            m_gcode_trace->colorless_indices_interval(ExtrusionRole(role_idx));
        int start_offset = index_offset.first;
        int size = index_offset.second - index_offset.first;

        mesh->index_offset = start_offset;
        mesh->index_count = size;
    }
}

void GcodePreview::renderGui()
{
    if (!m_loaded) {
        return;
    }

    if (ImGui::Begin("Gcode Controls")) {
        ImGui::TextUnformatted(m_source_filepath.c_str());

        const std::array<int, 2>& layer_range = m_gcode_trace->get_layer_range();
        const std::array<int, 2>& layer_scope = m_gcode_trace->get_layer_scope();
        int layer_min = layer_range[0];
        int layer_max = layer_range[1];
        int layer_low = layer_scope[0];
        int layer_high = layer_scope[1];
        bool layer_changed = false;
        layer_changed |= ImGui::SliderInt("Layer Bottom", &layer_low, layer_min, layer_max);
        if (layer_high < layer_low) {
            layer_high = layer_low;
        }
        layer_changed |= ImGui::SliderInt("Layer Top", &layer_high, layer_min, layer_max);
        if (layer_low > layer_high) {
            layer_low = layer_high;
        }
        if (layer_changed) {
            m_gcode_trace->set_layer_scope({ layer_low, layer_high });
        }

        const std::array<int, 2>& move_range = m_gcode_trace->get_move_range();
        const std::array<int, 2>& move_scope = m_gcode_trace->get_move_scope();
        int move_min = move_range[0];
        int move_max = move_range[1];
        int move_high = move_scope[1];
        bool move_changed = false;
        move_changed |= ImGui::SliderInt("Move", &move_high, move_min, move_max);
        if (move_changed) {
            m_gcode_trace->set_move_scope({ move_min , move_high });
        }

        ImGui::Separator();

        ImGui::BeginChild("Legend", ImVec2(0.0f, 0.0f), 0,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        auto append_item = [this](
            const Color4& color,
            const std::string& text,
            bool visible,
            std::function<void()> callback = nullptr)
        {
            ImVec2 icon_pos = ImVec2(ImGui::GetCursorScreenPos().x + 12, ImGui::GetCursorScreenPos().y);
            float icon_size = ImGui::CalcTextSize("A").y - 2.0f;
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled({ icon_pos.x, icon_pos.y + 1.f }, { icon_pos.x + icon_size, icon_pos.y + icon_size + 1.f },
                ImGui::GetColorU32({ color[0], color[1], color[2], color[3] }));

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20.0, 6.0));

            if (callback) {
                if (ImGui::Selectable(("##" + text).c_str(), visible, 0))
                    callback();

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0, 0.0));
                ImGui::SameLine(ImGui::GetWindowWidth() - 40);
                if (ImGui::Checkbox(("##" + text).c_str(), &visible)) {
                    callback();
                }
                ImGui::PopStyleVar(1);
            }

            {
                float dummy_size = ImGui::GetStyle().ItemSpacing.x + icon_size;
                ImGui::SameLine(dummy_size);
                ImGui::Text(text.c_str());
            }

            ImGui::PopStyleVar(1);
        };

        for (size_t i = 1; i < ExtrusionRole::erCount; ++i) {
            append_item(Extrusion_Role_Colors[i], role_labels[i],
                m_gcode_trace->is_visible(ExtrusionRole(i)),
                [this, i]() {
                    bool visible = m_gcode_trace->is_visible(ExtrusionRole(i));
                    visible = !visible;
                    m_gcode_trace->set_visible(ExtrusionRole(i), visible);
                });
        }
        ImGui::EndChild();

        if (m_gcode_trace->dirty()) {
            rebuildMeshRange(m_gcode_trace->linesBatches());
            m_gcode_trace->setDirty(false);
        }
    }
    ImGui::End();
}
