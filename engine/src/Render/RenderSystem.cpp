#include "RenderSystem.hpp"

#include "Path/ForwardRenderPath.hpp"
#include "Path/DeferredRenderPath.hpp"
#include "Path/RayTracingRenderPath.hpp"

#include <Logical/Framework/World/Scene.hpp>
#include <unordered_set>

RenderSystem::RenderSystem()
{
    RenderSourceData::initRHI();

    m_render_source_data = std::make_shared<RenderSourceData>();

    m_forward_path = std::make_shared<ForwardRenderPath>(this);
    m_deferred_path = std::make_shared<DeferredRenderPath>(this);

    m_render_params.render_path_type = RenderPathType::Forward;
    m_curr_path = m_forward_path;
}

RenderParams &RenderSystem::renderParams()
{
    return m_render_params;
}

unsigned int RenderSystem::getPickingFBO()
{
    return m_curr_path->getPickingFBO();
}

unsigned int RenderSystem::renderPassTexture(RenderPass::Type render_pass_type)
{
    RhiTexture *texture = m_curr_path->renderPassTexture(render_pass_type);
    return texture ? texture->id() : 0;
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
    m_curr_path->prepareRenderSourceData(m_render_source_data);
    m_curr_path->render();
}

void RenderSystem::updateRenderSourceData(std::shared_ptr<Scene> scene)
{
    if (!m_initialized)
    {
        std::shared_ptr<Mesh> screen_quad_sub_mesh;
        screen_quad_sub_mesh = Mesh::create_screen_mesh();
        m_render_source_data->screen_quad = std::make_shared<RenderMeshData>(screen_quad_sub_mesh);
    }

    if (!m_initialized)
    {
        const auto &dir_light = scene->getLightManager()->mainDirectionalLight();
        m_render_source_data->render_directional_light_data_list.emplace_back(
            RenderDirectionalLightData{dir_light->luminousColor, dir_light->direction,
                                       dir_light->lightViewMatrix(), dir_light->lightProjMatrix()});
    }

    const auto &point_lights = scene->getLightManager()->pointLights();
    struct inst_data
    {
        Mat4 inst_matrix;
        Color4 inst_color;
    };
    static inst_data *point_light_inst_data = new inst_data[point_lights.size()]{};
    if (!m_initialized)
    {
        for (int i = 0; i < point_lights.size(); ++i)
        {
            point_light_inst_data[i].inst_matrix = Math::Translate(point_lights[i]->position);
            point_light_inst_data[i].inst_color = point_lights[i]->luminousColor;
        }

        // std::shared_ptr<Mesh> point_light_mesh = Mesh::create_icosphere_mesh(0.05f, 4);
        // m_render_source_data->render_point_light_inst_mesh = std::make_shared<RenderMeshNode>(RenderMeshDataID(-99999, 0), point_light_mesh, Mat4(1.0));
        // m_render_source_data->render_point_light_inst_mesh->create_instancing(point_light_inst_data, point_lights.size() * sizeof(inst_data));
        m_render_source_data->point_light_inst_amount = point_lights.size();
    }

    auto &render_point_lights = m_render_source_data->render_point_light_data_list;
    render_point_lights.erase(
        std::remove_if(render_point_lights.begin(), render_point_lights.end(),
                       [&point_lights](const RenderPointLightData &render_point_light_data)
                       {
                           auto it = std::find_if(point_lights.begin(), point_lights.end(),
                                                  [&render_point_light_data](const auto &point_light)
                                                  { return point_light->ID().id == render_point_light_data.id; });
                           return it == point_lights.end();
                       }),
        render_point_lights.end());
    for (const auto &point_light : point_lights)
    {
        Mat4 point_light_matrix = Math::Translate(point_light->position);
        int point_light_id = point_light->ID().id;
        auto it = std::find_if(m_render_source_data->render_point_light_data_list.begin(), m_render_source_data->render_point_light_data_list.end(),
                               [point_light_id](const RenderPointLightData &point_light_data)
                               {
                                   return point_light_data.id == point_light_id;
                               });
        if (it != m_render_source_data->render_point_light_data_list.end())
        {
            it->position = point_light->position;
        }
        else
        {
            m_render_source_data->render_point_light_data_list.emplace_back(
                RenderPointLightData{point_light_id, point_light->luminousColor, point_light->position, point_light->radius,
                                     point_light->lightViewMatrix(), point_light->lightProjMatrix()});
        }
    }

    if (!m_initialized)
    {
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
    }

    const auto &objects = scene->getObjects();
    std::unordered_set<int> alive_object_ids;
    alive_object_ids.reserve(objects.size());
    for (const auto &object : objects)
    {
        alive_object_ids.insert(object->ID().id);
        auto &sub_meshes = object->getComponent<MeshComponent>()->sub_meshes;
        auto &model_matrix = object->getComponent<TransformComponent>()->transform();
        bool visible = object->visible();
        for (const auto &sub_mesh : sub_meshes)
        {
            auto render_mesh_data_id = RenderMeshNodeID(object->ID(), sub_mesh->sub_mesh_idx);
            auto it = m_render_source_data->render_mesh_nodes.find(render_mesh_data_id);
            if (!visible)
            {
                if (it != m_render_source_data->render_mesh_nodes.end())
                    m_render_source_data->render_mesh_nodes.erase(render_mesh_data_id);
            }
            else
            {
                if (it != m_render_source_data->render_mesh_nodes.end())
                {
                    m_render_source_data->render_mesh_nodes[render_mesh_data_id]->model_matrix = (model_matrix * sub_mesh->local_transform);
                    m_render_source_data->render_mesh_nodes[render_mesh_data_id]->updateRenderMaterialData(sub_mesh->material);
                }
                else
                {
                    m_render_source_data->render_mesh_nodes.insert_or_assign(render_mesh_data_id,
                                                                             std::make_shared<RenderMeshNode>(render_mesh_data_id, RenderMeshData(sub_mesh), RenderMaterialData(sub_mesh->material), model_matrix));
                }
            }
        }
    }
    // Remove stale render nodes whose owner object has been deleted from scene.
    auto &render_mesh_nodes = m_render_source_data->render_mesh_nodes;
    for (auto it = render_mesh_nodes.begin(); it != render_mesh_nodes.end();)
    {
        if (alive_object_ids.find(it->first.object_id.id) == alive_object_ids.end())
            it = render_mesh_nodes.erase(it);
        else
            ++it;
    }

    // picked
    m_render_source_data->picked_ids.clear();
    for (const auto &object : scene->getPickedObjects())
    {
        m_render_source_data->picked_ids.push_back(object->ID());
    }

    CameraComponent &camera = scene->getMainCamera();
    m_render_source_data->camera_position = camera.pos;
    m_render_source_data->view_matrix = camera.view;
    m_render_source_data->proj_matrix = camera.projection;

    if (!m_render_source_data->render_camera)
        m_render_source_data->render_camera = std::make_shared<RenderCameraData>();
    m_render_source_data->render_camera->fov = camera.fov;
    m_render_source_data->render_camera->pos = camera.pos;
    m_render_source_data->render_camera->direction = camera.direction;
    m_render_source_data->render_camera->upDirection = camera.upDirection;
    m_render_source_data->render_camera->rightDirection = camera.getRightDirection();

    m_initialized = true;
}
