#include "MaterialBallTest.hpp"
#include "Engine.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "AssetManager/Mesh.hpp"
#include "AssetManager/Material.hpp"

static const int   GRID_ROWS = 7;
static const int   GRID_COLS = 7;
static const float BALL_SPACING = 2.5f;
static const Vec3  BALL_ALBEDO = Vec3(1.0f, 0.86f, 0.57f); // gold

void MaterialBallTest::init()
{
    auto& engine = Engine::get();
    auto scene = engine.Scene();
    if (!scene)
        return;

    createLighting(scene.get());
    createMaterialBallGrid(scene.get());
    createShadowTestObjects(scene.get());
    createNanosuitModel(scene.get());
}

void MaterialBallTest::createMaterialBallGrid(Scene* scene)
{
    const float grid_origin_x = 0.0f;
    const float grid_origin_y = 1.5f;
    const float grid_origin_z = 0.0f;

    for (int row = 0; row < GRID_ROWS; ++row)
    {
        float roughness = static_cast<float>(row) / (GRID_ROWS - 1);

        for (int col = 0; col < GRID_COLS; ++col)
        {
            float metallic = static_cast<float>(col) / (GRID_COLS - 1);

            std::string ball_name = "Ball_R" + std::to_string(row) + "_M" + std::to_string(col);
            GObject* ball = scene->loadModel(ASSET_DIRECTORY + "/model/basic/sphere.obj");
            if (!ball)
                continue;

            ball->setName(ball_name);

            TransformComponent* transform = ball->getComponent<TransformComponent>();
            if (transform)
            {
                transform->translation = Vec3(
                    grid_origin_x + col * BALL_SPACING,
                    grid_origin_y + row * BALL_SPACING,
                    grid_origin_z
                );
            }

            MeshComponent* mesh_comp = ball->getComponent<MeshComponent>();
            if (mesh_comp && !mesh_comp->sub_meshes.empty())
            {
                auto mat = Material::create_complete_default_material();
                mat->base_color_factor = BALL_ALBEDO;
                mat->metallic_factor = metallic;
                mat->roughness_factor = roughness;
                mat->ao_factor = 1.0f;
                mat->fillBlinnPhongFromPBR();
                mesh_comp->sub_meshes[0]->material = mat;
                ball->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Mesh));
            }
        }
    }
}

void MaterialBallTest::createShadowTestObjects(Scene* scene)
{
    // ground plane
    {
        GObject* ground = scene->loadModel(ASSET_DIRECTORY + "/model/basic/cube.obj");
        if (ground)
        {
            ground->setName("Ground");
            TransformComponent* t = ground->getComponent<TransformComponent>();
            if (t)
            {
                t->translation = Vec3(7.5f, -0.5f, 0.0f);
                t->scale = Vec3(30.0f, 0.01f, 30.0f);
            }
            MeshComponent* mc = ground->getComponent<MeshComponent>();
            if (mc && !mc->sub_meshes.empty())
            {
                auto mat = Material::create_complete_default_material();
                mat->base_color_factor = Vec3(0.8f, 0.8f, 0.8f);
                mat->metallic_factor = 0.0f;
                mat->roughness_factor = 0.9f;
                mat->fillBlinnPhongFromPBR();
                mc->sub_meshes[0]->material = mat;
                ground->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Mesh));
            }
        }
    }

    // shadow-casting cubes at different heights
    struct CubeDef {
        Vec3 position;
        Vec3 scale;
        Vec3 color;
        const char* name;
    };
    CubeDef cubes[] = {
        { Vec3(-5.0f, 1.0f,  3.0f), Vec3(1.0f, 2.0f, 1.0f), Vec3(0.9f, 0.2f, 0.2f), "ShadowCube_Tall" },
        { Vec3(-3.0f, 0.5f,  5.0f), Vec3(1.0f, 1.0f, 1.0f), Vec3(0.2f, 0.9f, 0.2f), "ShadowCube_Mid"  },
        { Vec3(-5.0f, 0.25f, 7.0f), Vec3(2.0f, 0.5f, 2.0f), Vec3(0.2f, 0.2f, 0.9f), "ShadowCube_Flat" },
    };

    for (const auto& def : cubes)
    {
        GObject* cube = scene->loadModel(ASSET_DIRECTORY + "/model/basic/cube.obj");
        if (!cube)
            continue;

        cube->setName(def.name);
        TransformComponent* t = cube->getComponent<TransformComponent>();
        if (t)
        {
            t->translation = def.position;
            t->scale = def.scale;
        }

        MeshComponent* mc = cube->getComponent<MeshComponent>();
        if (mc && !mc->sub_meshes.empty())
        {
            auto mat = Material::create_complete_default_material();
            mat->base_color_factor = def.color;
            mat->metallic_factor = 0.0f;
            mat->roughness_factor = 0.5f;
            mat->fillBlinnPhongFromPBR();
            mc->sub_meshes[0]->material = mat;
            cube->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Mesh));
        }
    }
}

void MaterialBallTest::createLighting(Scene* scene)
{
    // Directional light with HDR intensity to test tone mapping
    GObject* dir_light = scene->createDirectionalLight("TestDirLight");
    if (dir_light)
    {
        auto* light = dir_light->getComponent<DirectionalLightComponent>();
        if (light)
        {
            light->luminousColor = Color3(3.0f, 3.0f, 3.0f);
            light->direction = Vec3(15.0f, -30.0f, 15.0f);
        }
    }

    GObject* point_light = scene->createPointLight("TestPointLight");
    if (point_light)
    {
        auto* t = point_light->getComponent<TransformComponent>();
        if (t)
            t->translation = Vec3(7.5f, 10.0f, -5.0f);

        auto* light = point_light->getComponent<PointLightComponent>();
        if (light)
        {
            light->luminousColor = Color3(2.0f, 2.0f, 2.0f);
            light->radius = 50.0f;
        }
    }
}

void MaterialBallTest::createNanosuitModel(Scene* scene)
{
    GObject* nanosuit = scene->loadModel(ASSET_DIRECTORY + "/model/nanosuit/nanosuit.obj");
    if (!nanosuit)
        return;

    nanosuit->setName("Nanosuit_NormalMapTest");
    TransformComponent* t = nanosuit->getComponent<TransformComponent>();
    if (t)
    {
        t->translation = Vec3(-8.0f, 0.0f, 0.0f);
        t->scale = Vec3(0.4f);
    }
}
