#include "RenderSystem.hpp"

#include "Render/RHI/rhi.hpp"

#include "Path/ForwardRenderPath.hpp"
#include "Path/DeferredRenderPath.hpp"
#include "Path/RayTracingRenderPath.hpp"

#include <Logical/Framework/World/Scene.hpp>
#include <Logical/Animation/AnimationSystem.hpp>

#include "GlobalContext.hpp"

RenderSystem::RenderSystem()
{
    Rhi::create();

    m_render_source_data = std::make_shared<RenderSourceData>();

    m_forward_path = std::make_shared<ForwardRenderPath>(this);
    m_deferred_path = std::make_shared<DeferredRenderPath>(this);

    m_curr_path = m_forward_path;

    rebuildRenderTargets();
}

RenderParams &RenderSystem::renderParams()
{
    return m_render_params;
}

unsigned int RenderSystem::renderGraphTextureOf(const std::string& resource_name)
{
    RhiTexture* texture = m_curr_path->renderGraphTextureOf(resource_name);
    return texture ? texture->id() : 0;
}

bool RenderSystem::readRenderGraphPixelRGBAOf(const std::string& resource_name, int x, int y, unsigned char out_rgba[4])
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

    updateRenderSourceData(scene);
    m_curr_path->render(*m_render_source_data);
}

void RenderSystem::updateRenderSourceData(std::shared_ptr<Scene> scene)
{
    const auto& main_dir_light = scene->getLightManager()->mainDirectionalLight();
    const auto& point_lights = scene->getLightManager()->pointLights();
    const auto& objects = scene->getObjects();

    if (!m_initialized)
    {
        // 初始化 screen_quad mesh
        std::shared_ptr<Mesh> screen_quad_sub_mesh;
        screen_quad_sub_mesh = Mesh::create_screen_mesh();
        m_render_source_data->screen_quad = std::make_shared<RenderMeshData>(screen_quad_sub_mesh);

        // 初始化 定向光源
        m_render_source_data->render_directional_light_data_list.emplace_back(
            RenderDirectionalLightData{ main_dir_light->luminousColor, main_dir_light->direction,
                                       main_dir_light->lightViewMatrix(), main_dir_light->lightProjMatrix()});

        // 初始化 render_point_light_inst_mesh
         std::shared_ptr<Mesh> point_light_mesh = Mesh::create_icosphere_mesh(0.1f, 4);
         m_render_source_data->render_point_light_inst_mesh = std::make_shared<RenderMeshData>(point_light_mesh);

        // 初始化 天空盒
        std::shared_ptr<Mesh> skybox_mesh = Mesh::create_cube_mesh();
        const std::string resource_dir = RESOURCE_DIR;
        CubeTexture skybox_cube_texture = CubeTexture(
            resource_dir + "/images/skybox/right.jpg",
            resource_dir + "/images/skybox/left.jpg",
            resource_dir + "/images/skybox/top.jpg",
            resource_dir + "/images/skybox/bottom.jpg",
            resource_dir + "/images/skybox/front.jpg",
            resource_dir + "/images/skybox/back.jpg");
        m_render_source_data->render_skybox_node.skybox_cube_map = RenderTextureData(skybox_cube_texture).id;
        m_render_source_data->render_skybox_node.mesh = std::make_shared<RenderMeshData>(skybox_mesh);

        m_initialized = true;
    }

    // 更新定向光源状态
    auto& render_dir_lights = m_render_source_data->render_directional_light_data_list;
    render_dir_lights[0].color = main_dir_light->luminousColor;
    render_dir_lights[0].direction = main_dir_light->direction;
    render_dir_lights[0].lightViewMatrix = main_dir_light->lightViewMatrix();
    render_dir_lights[0].lightProjMatrix = main_dir_light->lightProjMatrix();

    // 更新点光源状态
    struct PointLightInstData
    {
        Mat4 inst_matrix;
        Color3 inst_color;
    };
    m_render_source_data->point_light_inst_amount = point_lights.size();
    if (m_render_source_data->render_point_light_inst_mesh && !point_lights.empty())
    {
        static std::vector<PointLightInstData> point_light_inst_data;
        point_light_inst_data.resize(point_lights.size());
        for (size_t i = 0; i < point_lights.size(); ++i)
        {
            const auto& point_light = point_lights[i];
            point_light_inst_data[i].inst_matrix = Math::Translate(point_light->position);
            point_light_inst_data[i].inst_color = point_light->luminousColor;
        }
        m_render_source_data->render_point_light_inst_mesh->update_instancing(
            point_light_inst_data.data(),
            static_cast<int>(point_light_inst_data.size() * sizeof(PointLightInstData)));
    }

    auto &render_point_lights = m_render_source_data->render_point_light_data_list;
    std::unordered_set<int> alive_point_light_ids;
    alive_point_light_ids.reserve(point_lights.size());
    for (const auto &point_light : point_lights)
    {
        alive_point_light_ids.insert(point_light->ID().id);

        int point_light_id = point_light->ID().id;
        auto it = std::find_if(render_point_lights.begin(), render_point_lights.end(),
            [&point_light_id](const RenderPointLightData& render_point_light_data)
            {return render_point_light_data.id == point_light_id; });
        if (it != render_point_lights.end())
        {
            auto &render_point_light = *it;
            render_point_light.color = point_light->luminousColor;
            render_point_light.position = point_light->position;
            render_point_light.radius = point_light->radius;
            render_point_light.lightViewMatrix = point_light->lightViewMatrix();
            render_point_light.lightProjMatrix = point_light->lightProjMatrix();
        }
        else
        {
            render_point_lights.emplace_back(
                RenderPointLightData{point_light_id, point_light->luminousColor, point_light->position, point_light->radius,
                                     point_light->lightViewMatrix(), point_light->lightProjMatrix()});
        }
    }
    render_point_lights.erase(
        std::remove_if(render_point_lights.begin(), render_point_lights.end(),
                       [&alive_point_light_ids](const RenderPointLightData &render_point_light_data)
                       { return alive_point_light_ids.find(render_point_light_data.id) == alive_point_light_ids.end(); }),
        render_point_lights.end());

    // 更新objects对象状态
    m_render_source_data->has_transparent = false;
    std::unordered_set<int> alive_object_ids;
    alive_object_ids.reserve(objects.size());
    auto &render_mesh_nodes = m_render_source_data->render_mesh_nodes;
    auto *animation_system = g_context.animation_system.get();
    for (const auto &object : objects)
    {
        const int object_id = object->ID().id;
        alive_object_ids.insert(object_id);

        const bool visible = object->visible();
        if (!visible)
        {
            for (auto it = render_mesh_nodes.begin(); it != render_mesh_nodes.end();)
            {
                if (it->first.object_id.id == object_id)
                    it = render_mesh_nodes.erase(it);
                else
                    ++it;
            }
            continue;
        }


        const auto& sub_meshes = object->getComponent<MeshComponent>()->sub_meshes;
        const Mat4 obj_transform = object->getComponent<TransformComponent>()->transform();
        const bool use_skinning = animation_system && animation_system->HasAnimation(object_id);
        const std::vector<Mat4>* bone_matrices = use_skinning ? &animation_system->GetFinalBoneMatrices(object_id) : nullptr;

        std::unordered_set<int> alive_sub_mesh_ids;
        alive_sub_mesh_ids.reserve(sub_meshes.size());
        for (const auto &sub_mesh : sub_meshes)
        {
            if (sub_mesh->material->alpha != 1.0f)
                m_render_source_data->has_transparent = true;
            alive_sub_mesh_ids.insert(sub_mesh->sub_mesh_idx);
            Mat4 sub_mesh_transform = Math::composeMatrix(sub_mesh->scale, sub_mesh->rotation, sub_mesh->translation);
            auto render_mesh_mode_id = RenderMeshNodeID(object->ID(), sub_mesh->sub_mesh_idx);
            auto it = render_mesh_nodes.find(render_mesh_mode_id);
            if (it != render_mesh_nodes.end())
            {
                auto &render_node = *(it->second);
                render_node.model_matrix = (obj_transform * sub_mesh_transform);
                render_node.source_index_offset = sub_mesh->index_offset;
                render_node.source_index_count = sub_mesh->index_count;
                render_node.updateRenderMaterialData(sub_mesh->material);
                render_node.use_skinning = use_skinning;
                if (bone_matrices)
                    render_node.bone_matrices = *bone_matrices;
                else
                    render_node.bone_matrices.clear();
            }
            else
            {
                auto render_node = std::make_shared<RenderMeshNode>(
                    render_mesh_mode_id,
                    RenderMeshData(sub_mesh),
                    RenderMaterialData(sub_mesh->material),
                    obj_transform * sub_mesh_transform,
                    sub_mesh->index_offset,
                    sub_mesh->index_count);
                render_node->use_skinning = use_skinning;
                if (bone_matrices)
                    render_node->bone_matrices = *bone_matrices;
                render_mesh_nodes.emplace(render_mesh_mode_id, render_node);
            }
        }
        // Remove not alive sub_mesh
        for (auto it = render_mesh_nodes.begin(); it != render_mesh_nodes.end();)
        {
            if (it->first.object_id.id == object_id &&
                alive_sub_mesh_ids.find(it->first.sub_mesh_idx) == alive_sub_mesh_ids.end())
            {
                it = render_mesh_nodes.erase(it);
            }
            else
                ++it;
        }
    }
    // Remove not alive object
    for (auto it = render_mesh_nodes.begin(); it != render_mesh_nodes.end();)
    {
        if (alive_object_ids.find(it->first.object_id.id) == alive_object_ids.end())
            it = render_mesh_nodes.erase(it);
        else
            ++it;
    }

    // 更新picked对象
    m_render_source_data->picked_ids.clear();
    for (const auto &object : scene->getPickedObjects())
    {
        m_render_source_data->picked_ids.push_back(object->ID());
    }

    // 更新相机状态
    CameraComponent &camera = scene->getMainCamera();
    m_render_source_data->camera_position = camera.pos;
    m_render_source_data->view_matrix = camera.view;
    m_render_source_data->proj_matrix = camera.projection;

    // 没什么用
    if (!m_render_source_data->render_camera)
        m_render_source_data->render_camera = std::make_shared<RenderCameraData>();
    m_render_source_data->render_camera->fov = camera.fov;
    m_render_source_data->render_camera->pos = camera.pos;
    m_render_source_data->render_camera->direction = camera.direction;
    m_render_source_data->render_camera->upDirection = camera.upDirection;
    m_render_source_data->render_camera->rightDirection = camera.getRightDirection();
}
