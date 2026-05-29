#include "RenderSystem.hpp"

#include "Render/RHI/rhi.hpp"

#include "Path/ForwardRenderPath.hpp"
#include "Path/DeferredRenderPath.hpp"
#include "Path/RayTracingRenderPath.hpp"

#include <Logical/Framework/World/Scene.hpp>
#include <Logical/Framework/World/SceneDirty.hpp>
#include <Logical/Animation/AnimationSystem.hpp>
#include <Logical/Framework/Component/LightComponent.hpp>
#include <Logical/Framework/Component/TransformComponent.hpp>

#include <AssetManager/MeshAlgorithm.hpp>

#include "Render/IBL/IBLPreprocessor.hpp"
#include "Base/Utils/PathService.hpp"

#include "GlobalContext.hpp"

#include <fstream>

RenderSystem::RenderSystem()
{
    Rhi::create();

    m_forward_path = std::make_shared<ForwardRenderPath>(this);
    m_deferred_path = std::make_shared<DeferredRenderPath>(this);

    m_curr_path = m_forward_path;

    rebuildRenderTargets();
}

RenderParams &RenderSystem::renderParams()
{
    return m_render_params;
}

GL_HANDLE RenderSystem::renderGraphTextureOf(const std::string &resource_name)
{
    RhiTexture *texture = m_curr_path->renderGraphTextureOf(resource_name);
    return texture ? texture->id() : 0;
}

bool RenderSystem::readRenderGraphPixelRGBAOf(const std::string &resource_name, int x, int y, unsigned char out_rgba[4])
{
    return m_curr_path && m_curr_path->readRenderGraphPixelRGBAOf(resource_name, x, y, out_rgba);
}

std::vector<std::string> RenderSystem::renderGraphResourceNames() const
{
    return m_curr_path ? m_curr_path->renderGraphResourceNames() : std::vector<std::string>{};
}

std::vector<RenderGraphResourceDebugInfo> RenderSystem::renderGraphResourceDebugInfos() const
{
    return m_curr_path ? m_curr_path->renderGraphResourceDebugInfos() : std::vector<RenderGraphResourceDebugInfo>{};
}

std::string RenderSystem::renderGraphDebugDump() const
{
    return m_curr_path ? m_curr_path->renderGraphDebugDump() : std::string{};
}

std::string RenderSystem::renderGraphExecutionDump() const
{
    return m_curr_path ? m_curr_path->renderGraphExecutionDump() : std::string{};
}

void RenderSystem::rebuildRenderTargets()
{
    const Vec2 sz = m_render_params.renderTargetPixels();
    m_forward_path->resizeRenderTargets(sz);
    m_deferred_path->resizeRenderTargets(sz);
}

void RenderSystem::onUpdate(std::shared_ptr<Scene> scene)
{
    switch (m_render_params.render_path_type)
    {
    case RenderPathType::Forward:
        m_curr_path = m_forward_path;
        break;
    case RenderPathType::Deferred:
        m_curr_path = m_deferred_path;
        break;
    default:
        break;
    }

    initializeRenderResources();
    syncRenderSceneChanges(*scene);
    updateSkinnedMeshSections();
    buildRenderFrameData(*scene);
    m_curr_path->render(m_render_scene, m_frame_data, m_builtin_resources);
}

void RenderSystem::initializeRenderResources()
{
    if (m_initialized)
        return;

    std::shared_ptr<Mesh> screen_quad_sub_mesh = MeshAlgorithm::create_screen_mesh();
    m_builtin_resources.screen_quad = std::make_shared<RenderMeshResource>(screen_quad_sub_mesh);

    std::shared_ptr<Mesh> point_light_mesh = MeshAlgorithm::create_icosphere_mesh(0.1f, 4);
    m_builtin_resources.point_light_inst_mesh = std::make_shared<RenderMeshResource>(point_light_mesh);

    std::shared_ptr<Mesh> skybox_mesh = MeshAlgorithm::create_cube_mesh();
    const std::string asset_dir = ASSET_DIR;
    std::shared_ptr<CubeTexture> skybox_cube_texture = std::make_shared<CubeTexture>(
        asset_dir + "/images/skybox/right.jpg",
        asset_dir + "/images/skybox/left.jpg",
        asset_dir + "/images/skybox/top.jpg",
        asset_dir + "/images/skybox/bottom.jpg",
        asset_dir + "/images/skybox/front.jpg",
        asset_dir + "/images/skybox/back.jpg");
    m_render_scene.skybox().skybox_cube_map = RenderTextureData(skybox_cube_texture).texture;
    m_render_scene.skybox().mesh = std::make_shared<RenderMeshResource>(skybox_mesh);

    buildIBLResources(asset_dir);

    m_initialized = true;
}

void RenderSystem::buildIBLResources(const std::string& asset_dir)
{
    IBLPreprocessor preprocessor;

    // 1) 优先使用配置的 HDR；2) 否则使用约定默认 HDR；3) 都不存在则从 skybox cube 派生。
    std::string hdr_path = m_render_params.ibl.env_hdr_path;
    if (!hdr_path.empty() && !PathService::isAbsolute(hdr_path))
        hdr_path = PathService::join(asset_dir, hdr_path);

    auto fileExists = [](const std::string& p) {
        if (p.empty())
            return false;
        std::ifstream f(p);
        return f.good();
    };

    if (!fileExists(hdr_path))
    {
        const std::string default_hdr = asset_dir + "/images/ibl/environment.hdr";
        hdr_path = fileExists(default_hdr) ? default_hdr : std::string();
    }

    bool built = false;
    if (!hdr_path.empty())
    {
        built = preprocessor.buildFromEquirectHDR(hdr_path, m_builtin_resources.ibl);
        if (built)
            m_render_scene.skybox().skybox_cube_map = m_builtin_resources.ibl.environment_cube;
    }

    if (!built)
    {
        // 回退：直接用 skybox 的 6 面 cubemap 派生 IBL（LDR 近似）。
        preprocessor.buildFromEnvironmentCube(m_render_scene.skybox().skybox_cube_map, m_builtin_resources.ibl);
    }
}

void RenderSystem::syncRenderSceneChanges(Scene& scene)
{
    const auto changes = scene.consumeChanges();
    bool section_lists_dirty = false;

    for (const SceneChange& change : changes)
    {
        if (HasSceneDirtyFlag(change.flags, SceneDirtyFlag::FullResync))
        {
            rebuildRenderSceneFromScene(scene);
            return;
        }

        if (HasSceneDirtyFlag(change.flags, SceneDirtyFlag::Removed))
        {
            m_render_scene.removeObjectProxy(change.object_id);
            section_lists_dirty = true;
            continue;
        }

        GObject* object = scene.objectOf(change.object_id);
        if (!object)
        {
            m_render_scene.removeObjectProxy(change.object_id);
            section_lists_dirty = true;
            continue;
        }

        if (HasSceneDirtyFlag(change.flags, SceneDirtyFlag::Created) ||
            HasSceneDirtyFlag(change.flags, SceneDirtyFlag::Mesh))
        {
            rebuildObjectRenderProxy(*object);
            section_lists_dirty = true;
            continue;
        }

        if (HasSceneDirtyFlag(change.flags, SceneDirtyFlag::Visibility))
        {
            m_render_scene.setObjectVisible(change.object_id, object->visible());
            section_lists_dirty = true;
        }

        if (HasSceneDirtyFlag(change.flags, SceneDirtyFlag::Transform))
        {
            if (auto* transform = object->getComponent<TransformComponent>())
                m_render_scene.updateObjectTransform(change.object_id, transform->transform());
        }

        if (HasSceneDirtyFlag(change.flags, SceneDirtyFlag::Material))
        {
            m_render_scene.updateObjectMaterials(*object);
            section_lists_dirty = true;
        }
    }

    if (section_lists_dirty)
        m_render_scene.rebuildMeshSectionLists();
}

void RenderSystem::rebuildRenderSceneFromScene(Scene& scene)
{
    m_render_scene.clearObjectProxies();
    for (const auto& object : scene.getObjects())
    {
        if (object)
            rebuildObjectRenderProxy(*object);
    }
    m_render_scene.rebuildMeshSectionLists();
}

void RenderSystem::rebuildObjectRenderProxy(GObject& object)
{
    const GObjectID object_id = object.ID();
    auto* mesh_component = object.getComponent<MeshComponent>();
    auto* transform_component = object.getComponent<TransformComponent>();
    if (!mesh_component || !transform_component)
    {
        m_render_scene.removeObjectProxy(object_id);
        return;
    }

    m_render_scene.removeObjectProxy(object_id);

    const Mat4 obj_transform = transform_component->transform();
    const bool use_skinning = g_context.animation_system && g_context.animation_system->HasAnimation(object_id);
    const std::vector<Mat4>* bone_matrices = use_skinning ? &g_context.animation_system->GetFinalBoneMatrices(object_id) : nullptr;

    for (const auto& sub_mesh : mesh_component->sub_meshes)
    {
        if (!sub_mesh || !sub_mesh->material)
            continue;

        const Mat4 sub_mesh_transform = Math::composeMatrix(sub_mesh->scale, sub_mesh->rotation, sub_mesh->translation);
        const RenderMeshSectionID section_id(object.ID(), sub_mesh->sub_mesh_idx);
        auto render_section = std::make_unique<RenderMeshSection>(
            section_id,
            RenderMeshResource(sub_mesh),
            RenderMaterialResource(sub_mesh->material),
            obj_transform * sub_mesh_transform,
            sub_mesh->index_offset,
            sub_mesh->index_count);
        render_section->local_matrix = sub_mesh_transform;
        render_section->visible = object.visible();
        render_section->use_skinning = use_skinning;
        if (bone_matrices)
            render_section->bone_matrices = *bone_matrices;
        m_render_scene.addMeshSection(section_id, std::move(render_section));
    }

    if (RenderObjectProxy* object_proxy = m_render_scene.objectProxy(object_id))
    {
        object_proxy->setVisible(object.visible());
        object_proxy->setModelMatrix(obj_transform);
    }
}

void RenderSystem::updateSkinnedMeshSections()
{
    auto* animation_system = g_context.animation_system.get();
    if (!animation_system)
        return;

    for (RenderMeshSection* section : m_render_scene.skinnedMeshSections())
    {
        if (!section)
            continue;

        const GObjectID object_id = section->section_id.object_id;
        if (!animation_system->HasAnimation(object_id))
        {
            section->use_skinning = false;
            section->bone_matrices.clear();
            continue;
        }

        section->use_skinning = true;
        section->bone_matrices = animation_system->GetFinalBoneMatrices(object_id);
    }
}

void RenderSystem::buildRenderFrameData(Scene& scene)
{
    m_frame_data.reset();

    auto& render_dir_lights = m_frame_data.directional_lights;
    for (const GObjectID& light_id : scene.directionalLightObjectIDs())
    {
        GObject* light_object = scene.objectOf(light_id);
        if (!light_object || !light_object->visible())
            continue;
        const auto* directional_light = light_object->getComponent<DirectionalLightComponent>();
        if (!directional_light)
            continue;
        render_dir_lights.emplace_back(RenderDirectionalLightData{
            directional_light->luminousColor,
            directional_light->direction,
            directional_light->lightViewMatrix(),
            directional_light->lightProjMatrix()});
    }
    if (render_dir_lights.empty())
    {
        DirectionalLightComponent fallback_light(nullptr);
        fallback_light.luminousColor = Color3(0.0f);
        render_dir_lights.emplace_back(RenderDirectionalLightData{
            fallback_light.luminousColor,
            fallback_light.direction,
            fallback_light.lightViewMatrix(),
            fallback_light.lightProjMatrix()});
    }

    struct PointLightInstData
    {
        Mat4 inst_matrix;
        Color3 inst_color;
    };

    std::vector<GObject*> active_point_light_objects;
    active_point_light_objects.reserve(scene.pointLightObjectIDs().size());
    for (const GObjectID& light_id : scene.pointLightObjectIDs())
    {
        GObject* light_object = scene.objectOf(light_id);
        if (!light_object || !light_object->visible())
            continue;
        if (!light_object->getComponent<PointLightComponent>() || !light_object->getComponent<TransformComponent>())
            continue;
        active_point_light_objects.push_back(light_object);
    }

    m_frame_data.point_light_inst_amount = static_cast<int>(active_point_light_objects.size());
    if (m_builtin_resources.point_light_inst_mesh && !active_point_light_objects.empty())
    {
        static std::vector<PointLightInstData> point_light_inst_data;
        point_light_inst_data.resize(active_point_light_objects.size());
        for (size_t i = 0; i < active_point_light_objects.size(); ++i)
        {
            auto* light_object = active_point_light_objects[i];
            const auto* transform = light_object->getComponent<TransformComponent>();
            const auto* point_light = light_object->getComponent<PointLightComponent>();
            point_light_inst_data[i].inst_matrix = Math::Translate(transform->translation);
            point_light_inst_data[i].inst_color = point_light->luminousColor;
        }
        m_builtin_resources.point_light_inst_mesh->update_instancing(
            point_light_inst_data.data(),
            static_cast<int>(point_light_inst_data.size() * sizeof(PointLightInstData)));
    }

    auto& render_point_lights = m_frame_data.point_lights;
    render_point_lights.reserve(active_point_light_objects.size());
    for (auto* light_object : active_point_light_objects)
    {
        const auto* transform = light_object->getComponent<TransformComponent>();
        const auto* point_light = light_object->getComponent<PointLightComponent>();
        const Vec3 position = transform->translation;
        render_point_lights.emplace_back(RenderPointLightData{
            light_object->ID().value(),
            point_light->luminousColor,
            position,
            point_light->radius,
            point_light->lightViewMatrix(position),
            point_light->lightProjMatrix()});
    }

    for (const auto& object : scene.getPickedObjects())
    {
        if (object)
            m_frame_data.picked_ids.push_back(object->ID());
    }

    CameraComponent& camera = scene.getMainCamera();
    m_frame_data.camera_position = camera.pos;
    m_frame_data.view_matrix = camera.view;
    m_frame_data.proj_matrix = camera.projection;

    if (!m_frame_data.render_camera)
        m_frame_data.render_camera = std::make_shared<RenderCameraData>();
    m_frame_data.render_camera->fov = camera.fov;
    m_frame_data.render_camera->pos = camera.pos;
    m_frame_data.render_camera->direction = camera.direction;
    m_frame_data.render_camera->upDirection = camera.upDirection;
    m_frame_data.render_camera->rightDirection = camera.getRightDirection();
}
