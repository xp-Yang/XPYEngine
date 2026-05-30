#include "MaterialBallTest.hpp"
#include "Engine.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "AssetManager/Mesh.hpp"
#include "AssetManager/Material.hpp"
#include "AssetManager/Texture.hpp"

#include <json11.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace {

bool readJsonFile(const std::string& path, json11::Json& out)
{
    std::ifstream fin(path);
    if (!fin)
        return false;
    std::stringstream ss;
    ss << fin.rdbuf();
    std::string err;
    out = json11::Json::parse(ss.str(), err);
    return err.empty();
}

// 在材质目录中按显式文件名或常见命名探测一张贴图，返回绝对路径或空串。
std::string resolveTexture(const std::string& dir,
                           const std::string& explicit_name,
                           std::initializer_list<const char*> candidates)
{
    if (dir.empty())
        return {};
    if (!explicit_name.empty()) {
        std::string p = dir + "/" + explicit_name;
        if (fs::exists(p))
            return p;
    }
    for (const char* c : candidates) {
        std::string p = dir + "/" + c;
        if (fs::exists(p))
            return p;
    }
    return {};
}

// 由 material.json 与目录贴图构建 PBR 材质。
std::shared_ptr<Material> loadMaterialFromDir(const std::string& dir)
{
    auto mat = Material::create_complete_default_material();

    std::string albedo_name, metallic_name, roughness_name, normal_name, ao_name;
    const std::string json_path = dir + "/material.json";
    json11::Json json;
    if (fs::exists(json_path) && readJsonFile(json_path, json)) {
        const auto& bc = json["base_color_factor"].array_items();
        if (bc.size() == 3)
            mat->base_color_factor = Vec3((float)bc[0].number_value(), (float)bc[1].number_value(), (float)bc[2].number_value());
        if (json["metallic_factor"].is_number())
            mat->metallic_factor = (float)json["metallic_factor"].number_value();
        if (json["roughness_factor"].is_number())
            mat->roughness_factor = (float)json["roughness_factor"].number_value();
        if (json["ao_factor"].is_number())
            mat->ao_factor = (float)json["ao_factor"].number_value();
        albedo_name    = json["albedo_map"].string_value();
        metallic_name  = json["metallic_map"].string_value();
        roughness_name = json["roughness_map"].string_value();
        normal_name    = json["normal_map"].string_value();
        ao_name        = json["ao_map"].string_value();
    }

    const std::string albedo = resolveTexture(dir, albedo_name, { "albedo.png", "albedo.jpg", "basecolor.png", "diffuse.png" });
    if (!albedo.empty())
        mat->albedo_texture = std::make_shared<Texture>(TextureType::Albedo, albedo, true);

    const std::string metallic_tex = resolveTexture(dir, metallic_name, { "metallic.png", "metallic.jpg" });
    if (!metallic_tex.empty())
        mat->metallic_texture = std::make_shared<Texture>(TextureType::Metallic, metallic_tex, false);

    const std::string roughness_tex = resolveTexture(dir, roughness_name, { "roughness.png", "roughness.jpg" });
    if (!roughness_tex.empty())
        mat->roughness_texture = std::make_shared<Texture>(TextureType::Roughness, roughness_tex, false);

    const std::string ao_tex = resolveTexture(dir, ao_name, { "ao.png", "ao.jpg" });
    if (!ao_tex.empty())
        mat->ao_texture = std::make_shared<Texture>(TextureType::AO, ao_tex, false);

    const std::string normal_tex = resolveTexture(dir, normal_name, { "normal.png", "normal.jpg" });
    if (!normal_tex.empty())
        mat->normal_texture = std::make_shared<Texture>(TextureType::Normal, normal_tex, false);

    mat->fillBlinnPhongFromPBR();
    return mat;
}

void assignMaterial(GObject* obj, const std::shared_ptr<Material>& mat)
{
    MeshComponent* mc = obj->getComponent<MeshComponent>();
    if (!mc)
        return;
    for (auto& sub : mc->sub_meshes) {
        if (sub)
            sub->material = mat;
    }
    obj->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Mesh));
}

} // namespace

void MaterialBallTest::init()
{
    auto& engine = Engine::get();
    auto scene = engine.Scene();
    if (!scene)
        return;

    m_root_dir = ASSET_DIRECTORY + "/material_ball";
    m_mesh_path = loadShaderBallMesh();

    createLighting(scene.get());
    createGround(scene.get());
    spawnGallery(scene.get());
}

std::string MaterialBallTest::loadShaderBallMesh() const
{
    const char* candidates[] = {
        "/mesh/shader_ball.obj",
        "/mesh/shader_ball.gltf",
        "/mesh/shader_ball.glb",
        "/mesh/shader_ball.fbx",
    };
    for (const char* c : candidates) {
        std::string p = m_root_dir + c;
        if (fs::exists(p))
            return p;
    }
    return ASSET_DIRECTORY + "/model/basic/sphere.obj";
}

std::map<std::string, std::shared_ptr<Material>> MaterialBallTest::loadMaterialLibrary() const
{
    std::map<std::string, std::shared_ptr<Material>> lib;
    const std::string materials_dir = m_root_dir + "/materials";

    if (fs::exists(materials_dir) && fs::is_directory(materials_dir)) {
        for (const auto& entry : fs::directory_iterator(materials_dir)) {
            if (!entry.is_directory())
                continue;
            const std::string name = entry.path().filename().string();
            lib[name] = loadMaterialFromDir(entry.path().string());
        }
    }

    return lib;
}

MaterialBallTest::GalleryLayout MaterialBallTest::loadLayout(const std::vector<std::string>& available) const
{
    GalleryLayout layout;

    json11::Json json;
    const std::string path = m_root_dir + "/layout.json";
    if (fs::exists(path) && readJsonFile(path, json)) {
        if (json["spacing"].is_number())
            layout.spacing = (float)json["spacing"].number_value();
        if (json["lift"].is_number())
            layout.lift = (float)json["lift"].number_value();
        if (json["scale"].is_number())
            layout.scale = (float)json["scale"].number_value();
        const auto& origin = json["origin"].array_items();
        if (origin.size() == 3)
            for (int i = 0; i < 3; ++i)
                layout.origin[i] = (float)origin[i].number_value();
        for (const auto& rc : json["row_counts"].array_items())
            layout.row_counts.push_back((int)rc.number_value());
        for (const auto& m : json["materials"].array_items()) {
            const std::string name = m.string_value();
            if (std::find(available.begin(), available.end(), name) != available.end())
                layout.materials.push_back(name);
        }
    }

    if (layout.materials.empty())
        layout.materials = available;

    // row_counts 缺失或容量不足时，按递增宽度自动生成三角阵列。
    int capacity = 0;
    for (int c : layout.row_counts)
        capacity += c;
    if (layout.row_counts.empty() || capacity < (int)layout.materials.size()) {
        layout.row_counts.clear();
        int placed = 0, width = 3;
        const int total = (int)layout.materials.size();
        while (placed < total) {
            int c = std::min(width, total - placed);
            layout.row_counts.push_back(c);
            placed += c;
            if (width < 7)
                ++width;
        }
    }

    return layout;
}

void MaterialBallTest::spawnGallery(Scene* scene)
{
    auto lib = loadMaterialLibrary();

    std::vector<std::string> available;
    available.reserve(lib.size());
    for (const auto& kv : lib)
        available.push_back(kv.first);

    const GalleryLayout layout = loadLayout(available);
    const float row_depth = layout.spacing * 0.9f;

    size_t mat_idx = 0;
    for (size_t r = 0; r < layout.row_counts.size() && mat_idx < layout.materials.size(); ++r) {
        const int count = layout.row_counts[r];
        for (int c = 0; c < count && mat_idx < layout.materials.size(); ++c) {
            const std::string& mat_name = layout.materials[mat_idx++];
            auto it = lib.find(mat_name);
            if (it == lib.end())
                continue;

            GObject* ball = scene->loadModel(m_mesh_path);
            if (!ball)
                continue;
            ball->setName("Ball_" + mat_name);

            if (TransformComponent* t = ball->getComponent<TransformComponent>()) {
                const float x = layout.origin[0] + (c - (count - 1) * 0.5f) * layout.spacing;
                const float z = layout.origin[2] - (float)r * row_depth;
                t->translation = Vec3(x, layout.origin[1] + layout.lift, z);
                t->scale = Vec3(layout.scale);
            }

            assignMaterial(ball, it->second);
        }
    }
}

void MaterialBallTest::createGround(Scene* scene)
{
    GObject* ground = scene->loadModel(ASSET_DIRECTORY + "/model/basic/cube.obj");
    if (!ground)
        return;

    ground->setName("GalleryGround");
    if (TransformComponent* t = ground->getComponent<TransformComponent>()) {
        t->translation = Vec3(0.0f, -0.5f, -10.0f);
        t->scale = Vec3(120.0f, 1.0f, 120.0f);
    }

    auto mat = Material::create_complete_default_material();
    mat->base_color_factor = Vec3(0.35f, 0.35f, 0.37f);
    mat->metallic_factor = 0.0f;
    mat->roughness_factor = 0.85f;
    mat->fillBlinnPhongFromPBR();
    assignMaterial(ground, mat);
}

void MaterialBallTest::createLighting(Scene* scene)
{
    // 主方向光：补出阴影形状，待 IBL 接入后可进一步弱化。
    if (GObject* dir_light = scene->createDirectionalLight("GalleryKeyLight")) {
        if (auto* light = dir_light->getComponent<DirectionalLightComponent>()) {
            light->luminousColor = Color3(1.5f, 1.5f, 1.5f);
            light->direction = Vec3(-0.4f, -1.0f, -0.5f);
        }
    }

    // 弱补光点光源，避免暗部死黑。
    if (GObject* point_light = scene->createPointLight("GalleryFillLight")) {
        if (auto* t = point_light->getComponent<TransformComponent>())
            t->translation = Vec3(0.0f, 12.0f, 8.0f);
        if (auto* light = point_light->getComponent<PointLightComponent>()) {
            light->luminousColor = Color3(1.5f, 1.5f, 1.5f);
            light->radius = 60.0f;
        }
    }
}
