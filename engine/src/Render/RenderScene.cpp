#include "Render/RenderScene.hpp"

#include "Logical/Framework/Component/LightComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace
{
RenderDirectionalLightData directionalLightDataOf(const RenderDirectionalLightProxy& proxy)
{
    return RenderDirectionalLightData{
        proxy.color,
        proxy.direction,
        proxy.light_view_matrix,
        proxy.light_proj_matrix
    };
}

RenderPointLightData pointLightDataOf(const RenderPointLightProxy& proxy, int shadow_index)
{
    return RenderPointLightData{
        proxy.object_id.value(),
        proxy.color,
        proxy.position,
        proxy.radius,
        shadow_index,
        proxy.light_view_matrices,
        proxy.light_proj_matrix
    };
}

RenderDirectionalLightData fallbackDirectionalLightData()
{
    DirectionalLightComponent fallback_light(nullptr);
    fallback_light.luminousColor = Color3(0.0f);
    return RenderDirectionalLightData{
        fallback_light.luminousColor,
        fallback_light.direction,
        fallback_light.lightViewMatrix(),
        fallback_light.lightProjMatrix()
    };
}

std::array<Mat4, 6> pointLightViewMatricesOf(const Vec3& position)
{
    return {
        Math::LookAt(position, position + Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)),
        Math::LookAt(position, position + Vec3(-1.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)),
        Math::LookAt(position, position + Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)),
        Math::LookAt(position, position + Vec3(0.0f, -1.0f, 0.0f), Vec3(0.0f, 0.0f, -1.0f)),
        Math::LookAt(position, position + Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, -1.0f, 0.0f)),
        Math::LookAt(position, position + Vec3(0.0f, 0.0f, -1.0f), Vec3(0.0f, -1.0f, 0.0f)),
    };
}
}

RenderObjectProxy* RenderScene::objectProxy(GObjectID object_id)
{
    auto it = m_object_proxies.find(object_id);
    return it == m_object_proxies.end() ? nullptr : it->second.get();
}

const RenderObjectProxy* RenderScene::objectProxy(GObjectID object_id) const
{
    auto it = m_object_proxies.find(object_id);
    return it == m_object_proxies.end() ? nullptr : it->second.get();
}

RenderMeshSection* RenderScene::meshSection(const RenderMeshSectionID& id)
{
    RenderObjectProxy* proxy = objectProxy(id.object_id);
    return proxy ? proxy->meshSection(id.sub_mesh_idx) : nullptr;
}

const RenderMeshSection* RenderScene::meshSection(const RenderMeshSectionID& id) const
{
    const RenderObjectProxy* proxy = objectProxy(id.object_id);
    return proxy ? proxy->meshSection(id.sub_mesh_idx) : nullptr;
}

RenderMeshSection* RenderScene::addMeshSection(const RenderMeshSectionID& id, std::unique_ptr<RenderMeshSection> section)
{
    if (!section)
        return nullptr;

    auto it = m_object_proxies.find(id.object_id);
    if (it == m_object_proxies.end())
        it = m_object_proxies.emplace(id.object_id, std::make_unique<RenderObjectProxy>(id.object_id)).first;

    return it->second->addMeshSection(std::move(section));
}

void RenderScene::removeObjectProxy(GObjectID object_id)
{
    if (const RenderObjectProxy* proxy = objectProxy(object_id))
    {
        if (proxy->hasVisibleStaticShadowCaster())
            ++m_shadow_static_version;
    }
    m_object_proxies.erase(object_id);
}

void RenderScene::removeDeadObjectProxies(const std::unordered_set<GObjectID>& alive_object_ids)
{
    for (auto it = m_object_proxies.begin(); it != m_object_proxies.end();)
    {
        if (alive_object_ids.find(it->first) == alive_object_ids.end())
        {
            if (it->second && it->second->hasVisibleStaticShadowCaster())
                ++m_shadow_static_version;
            it = m_object_proxies.erase(it);
        }
        else
            ++it;
    }
}

void RenderScene::removeDeadSubMeshesOfObject(GObjectID object_id, const std::unordered_set<int>& alive_sub_mesh_ids)
{
    RenderObjectProxy* proxy = objectProxy(object_id);
    if (!proxy)
        return;

    const bool affected_static_shadow = proxy->hasVisibleStaticShadowCaster();
    auto& mesh_sections = proxy->meshSections();
    mesh_sections.erase(
        std::remove_if(
            mesh_sections.begin(),
            mesh_sections.end(),
            [&alive_sub_mesh_ids](const std::unique_ptr<RenderMeshSection>& section)
            {
                return !section || alive_sub_mesh_ids.find(section->section_id.sub_mesh_idx) == alive_sub_mesh_ids.end();
            }),
        mesh_sections.end());

    if (mesh_sections.empty())
        removeObjectProxy(object_id);
    else if (affected_static_shadow)
        ++m_shadow_static_version;
}

void RenderScene::setObjectVisible(GObjectID object_id, bool visible)
{
    RenderObjectProxy* proxy = objectProxy(object_id);
    if (proxy)
    {
        const bool affected_static_shadow = proxy->hasVisibleStaticShadowCaster();
        proxy->setVisible(visible);
        if (affected_static_shadow || proxy->hasVisibleStaticShadowCaster())
            ++m_shadow_static_version;
    }
}

void RenderScene::updateObjectTransform(GObjectID object_id, const Mat4& model_matrix)
{
    RenderObjectProxy* proxy = objectProxy(object_id);
    if (proxy)
    {
        const bool affected_static_shadow = proxy->hasVisibleStaticShadowCaster();
        proxy->setModelMatrix(model_matrix);
        if (affected_static_shadow)
            ++m_shadow_static_version;
    }
}

void RenderScene::updateObjectMaterials(GObject& object)
{
    RenderObjectProxy* proxy = objectProxy(object.ID());
    MeshComponent* mesh_component = object.getComponent<MeshComponent>();
    if (!proxy || !mesh_component)
        return;

    // TODO: This round tracks mesh/material dirty at object granularity. Later, carry
    // sub_mesh_idx in RenderSceneChange so a material edit can refresh only the
    // affected RenderMeshSection.
    for (const auto& sub_mesh : mesh_component->sub_meshes)
    {
        if (!sub_mesh || !sub_mesh->material)
            continue;

        if (RenderMeshSection* section = proxy->meshSection(sub_mesh->sub_mesh_idx))
            section->updateRenderMaterial(sub_mesh->material);
    }
}

void RenderScene::syncLightProxy(GObject& object)
{
    const GObjectID object_id = object.ID();
    bool lights_dirty = false;
    bool point_light_instance_dirty = false;

    if (const auto* directional_light = object.getComponent<DirectionalLightComponent>())
    {
        RenderDirectionalLightProxy proxy;
        proxy.object_id = object_id;
        proxy.visible = object.visible();
        proxy.color = directional_light->luminousColor;
        proxy.direction = directional_light->direction;
        proxy.aspect_ratio = directional_light->aspectRatio;
        proxy.light_view_matrix = directional_light->lightViewMatrix();
        proxy.light_proj_matrix = directional_light->lightProjMatrix();
        m_directional_light_proxies[object_id] = proxy;
        lights_dirty = true;
    }
    else if (m_directional_light_proxies.erase(object_id) > 0)
    {
        lights_dirty = true;
    }

    const auto* point_light = object.getComponent<PointLightComponent>();
    const auto* transform = object.getComponent<TransformComponent>();
    if (point_light && transform)
    {
        RenderPointLightProxy proxy;
        proxy.object_id = object_id;
        proxy.visible = object.visible();
        proxy.color = point_light->luminousColor;
        proxy.position = transform->translation;
        proxy.radius = point_light->radius;
        proxy.cast_shadow = point_light->castShadow;
        proxy.light_view_matrices = point_light->lightViewMatrix(proxy.position);
        proxy.light_proj_matrix = point_light->lightProjMatrix();
        m_point_light_proxies[object_id] = proxy;
        lights_dirty = true;
        point_light_instance_dirty = true;
    }
    else if (m_point_light_proxies.erase(object_id) > 0)
    {
        lights_dirty = true;
        point_light_instance_dirty = true;
    }

    if (lights_dirty)
        m_light_lists_dirty = true;
    if (point_light_instance_dirty)
        m_point_light_instance_data_dirty = true;
}

void RenderScene::removeLightProxy(GObjectID object_id)
{
    const bool removed_directional = m_directional_light_proxies.erase(object_id) > 0;
    const bool removed_point = m_point_light_proxies.erase(object_id) > 0;
    if (removed_directional || removed_point)
        m_light_lists_dirty = true;
    if (removed_point)
        m_point_light_instance_data_dirty = true;
}

void RenderScene::clearLightProxies()
{
    m_directional_light_proxies.clear();
    m_point_light_proxies.clear();
    m_visible_directional_lights.clear();
    m_visible_point_lights.clear();
    m_cached_directional_light_data.clear();
    m_cached_point_light_data.clear();
    m_point_light_instance_data.clear();
    m_light_lists_dirty = true;
    m_point_light_instance_data_dirty = true;
}

void RenderScene::setLightVisible(GObjectID object_id, bool visible)
{
    bool lights_dirty = false;
    bool point_light_instance_dirty = false;

    auto directional_it = m_directional_light_proxies.find(object_id);
    if (directional_it != m_directional_light_proxies.end() && directional_it->second.visible != visible)
    {
        directional_it->second.visible = visible;
        lights_dirty = true;
    }

    auto point_it = m_point_light_proxies.find(object_id);
    if (point_it != m_point_light_proxies.end() && point_it->second.visible != visible)
    {
        point_it->second.visible = visible;
        lights_dirty = true;
        point_light_instance_dirty = true;
    }

    if (lights_dirty)
        m_light_lists_dirty = true;
    if (point_light_instance_dirty)
        m_point_light_instance_data_dirty = true;
}

void RenderScene::updateLightTransform(GObjectID object_id, const TransformComponent& transform)
{
    auto point_it = m_point_light_proxies.find(object_id);
    if (point_it == m_point_light_proxies.end())
        return;

    RenderPointLightProxy& proxy = point_it->second;
    proxy.position = transform.translation;
    proxy.light_view_matrices = pointLightViewMatricesOf(proxy.position);
    m_light_lists_dirty = true;
    m_point_light_instance_data_dirty = true;
}

void RenderScene::rebuildLightListsAndData()
{
    if (!m_light_lists_dirty)
        return;

    m_visible_directional_lights.clear();
    m_visible_point_lights.clear();
    m_cached_directional_light_data.clear();
    m_cached_point_light_data.clear();
    m_point_light_instance_data.clear();

    m_visible_directional_lights.reserve(m_directional_light_proxies.size());
    for (auto& pair : m_directional_light_proxies)
    {
        if (pair.second.visible)
            m_visible_directional_lights.push_back(&pair.second);
    }
    std::sort(
        m_visible_directional_lights.begin(),
        m_visible_directional_lights.end(),
        [](const RenderDirectionalLightProxy* lhs, const RenderDirectionalLightProxy* rhs)
        {
            return lhs && rhs ? lhs->object_id.value() < rhs->object_id.value() : lhs != nullptr;
        });

    m_cached_directional_light_data.reserve(m_visible_directional_lights.size());
    for (const RenderDirectionalLightProxy* light : m_visible_directional_lights)
    {
        if (light)
            m_cached_directional_light_data.push_back(directionalLightDataOf(*light));
    }
    if (m_cached_directional_light_data.empty())
        m_cached_directional_light_data.push_back(fallbackDirectionalLightData());

    m_visible_point_lights.reserve(m_point_light_proxies.size());
    for (auto& pair : m_point_light_proxies)
    {
        if (pair.second.visible)
            m_visible_point_lights.push_back(&pair.second);
    }
    std::sort(
        m_visible_point_lights.begin(),
        m_visible_point_lights.end(),
        [](const RenderPointLightProxy* lhs, const RenderPointLightProxy* rhs)
        {
            return lhs && rhs ? lhs->object_id.value() < rhs->object_id.value() : lhs != nullptr;
        });

    m_point_light_instance_data.reserve(m_visible_point_lights.size());
    for (const RenderPointLightProxy* light : m_visible_point_lights)
    {
        if (!light)
            continue;
        m_point_light_instance_data.push_back(RenderPointLightInstanceData{
            Math::Translate(light->position),
            light->color
        });
    }

    m_cached_point_light_data.reserve(std::min(MAX_CUBE_SHADOW_MAP_COUNT, m_visible_point_lights.size()));
    int next_shadow_index = 0;
    for (const RenderPointLightProxy* light : m_visible_point_lights)
    {
        if (!light || m_cached_point_light_data.size() >= MAX_CUBE_SHADOW_MAP_COUNT)
            break;

        int shadow_index = -1;
        if (light->cast_shadow && next_shadow_index < static_cast<int>(MAX_CUBE_SHADOW_MAP_COUNT))
            shadow_index = next_shadow_index++;

        m_cached_point_light_data.push_back(pointLightDataOf(*light, shadow_index));
    }

    m_light_lists_dirty = false;
}

void RenderScene::rebuildMeshSectionLists()
{
    m_visible_sections.clear();
    m_opaque_sections.clear();
    m_transparent_sections.clear();
    m_main_camera_visible_sections.clear();
    m_main_camera_opaque_sections.clear();
    m_main_camera_transparent_sections.clear();
    m_skinned_sections.clear();
    m_static_shadow_caster_sections.clear();
    m_dynamic_shadow_caster_sections.clear();
    m_has_transparent = false;
    m_main_camera_has_transparent = false;
    m_main_camera_culling_enabled = false;

    size_t mesh_section_count = 0;
    for (const auto& pair : m_object_proxies)
        mesh_section_count += pair.second ? pair.second->meshSections().size() : 0;

    m_visible_sections.reserve(mesh_section_count);
    m_opaque_sections.reserve(mesh_section_count);
    m_transparent_sections.reserve(mesh_section_count);
    m_skinned_sections.reserve(mesh_section_count);
    m_static_shadow_caster_sections.reserve(mesh_section_count);
    m_dynamic_shadow_caster_sections.reserve(mesh_section_count);

    for (auto& pair : m_object_proxies)
    {
        RenderObjectProxy* proxy = pair.second.get();
        if (!proxy || !proxy->visible())
            continue;

        for (auto& mesh_section : proxy->meshSections())
        {
            RenderMeshSection* section = mesh_section.get();
            if (!section || !section->visible)
                continue;

            m_visible_sections.push_back(section);
            if (section->material.isTransparent())
            {
                m_transparent_sections.push_back(section);
                m_has_transparent = true;
            }
            else
            {
                m_opaque_sections.push_back(section);
            }

            if (section->use_skinning)
                m_skinned_sections.push_back(section);

            if (section->static_shadow_caster && !section->use_skinning)
                m_static_shadow_caster_sections.push_back(section);
            else
                m_dynamic_shadow_caster_sections.push_back(section);
        }
    }
    ++m_shadow_static_version;
}

void RenderScene::updateMainCameraCulling(const RenderFrustum& frustum, bool enabled)
{
    m_main_camera_culling_enabled = enabled;
    m_main_camera_visible_sections.clear();
    m_main_camera_opaque_sections.clear();
    m_main_camera_transparent_sections.clear();
    m_main_camera_has_transparent = false;

    if (!enabled)
        return;

    m_main_camera_visible_sections.reserve(m_visible_sections.size());
    m_main_camera_opaque_sections.reserve(m_opaque_sections.size());
    m_main_camera_transparent_sections.reserve(m_transparent_sections.size());

    for (RenderMeshSection* section : m_visible_sections)
    {
        if (!section)
            continue;

        // Skinned vertices may move outside the static section bounds; keep them visible
        // until animated bounds or bone-aware bounds are available.
        if (!section->use_skinning && !frustum.intersects(section->world_bounds))
            continue;

        m_main_camera_visible_sections.push_back(section);
        if (section->material.isTransparent())
        {
            m_main_camera_transparent_sections.push_back(section);
            m_main_camera_has_transparent = true;
        }
        else
        {
            m_main_camera_opaque_sections.push_back(section);
        }
    }
}

void RenderScene::clearObjectProxies()
{
    m_object_proxies.clear();
    m_visible_sections.clear();
    m_opaque_sections.clear();
    m_transparent_sections.clear();
    m_main_camera_visible_sections.clear();
    m_main_camera_opaque_sections.clear();
    m_main_camera_transparent_sections.clear();
    m_skinned_sections.clear();
    m_static_shadow_caster_sections.clear();
    m_dynamic_shadow_caster_sections.clear();
    m_has_transparent = false;
    m_main_camera_has_transparent = false;
    m_main_camera_culling_enabled = false;
    ++m_shadow_static_version;
}

void RenderScene::clear()
{
    clearObjectProxies();
    clearLightProxies();
    m_skybox.mesh.reset();
    m_skybox.skybox_cube_map = nullptr;
    m_skybox.external_skybox_cube_map = nullptr;
}
