#ifndef RenderScene_hpp
#define RenderScene_hpp

#include "Logical/Framework/Component/TransformComponent.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "Render/RenderCulling.hpp"
#include "Render/RenderFrameData.hpp"
#include "Render/RenderProxy.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Long-lived render scene database. It owns render proxies and builds pass-facing section lists.
class RenderScene {
public:
    RenderObjectProxy* objectProxy(GObjectID object_id);
    const RenderObjectProxy* objectProxy(GObjectID object_id) const;

    // Find a mesh section by object id and subMesh index. This is a linear lookup inside the object.
    RenderMeshSection* meshSection(const RenderMeshSectionID& id);
    const RenderMeshSection* meshSection(const RenderMeshSectionID& id) const;

    // Create the object proxy if needed, then attach a new section to it.
    RenderMeshSection* addMeshSection(const RenderMeshSectionID& id, std::unique_ptr<RenderMeshSection> section);

    // Remove the object proxy and all sections owned by it.
    void removeObjectProxy(GObjectID object_id);

    // Remove proxies whose logical objects no longer exist.
    void removeDeadObjectProxies(const std::unordered_set<GObjectID>& alive_object_ids);

    // Remove sections whose subMeshes no longer exist on the logical object.
    void removeDeadSubMeshesOfObject(GObjectID object_id, const std::unordered_set<int>& alive_sub_mesh_ids);

    // Set object-level visibility and update owned sections.
    void setObjectVisible(GObjectID object_id, bool visible);

    // Update object transform and refresh all owned section model matrices.
    void updateObjectTransform(GObjectID object_id, const Mat4& model_matrix);

    // Refresh section material resources for all sections of one object.
    void updateObjectMaterials(GObject& object);

    // Rebuild visible/opaque/transparent/skinned lists consumed by render passes.
    void rebuildMeshSectionLists();

    // Rebuild the main-camera visible lists from the current section lists.
    void updateMainCameraCulling(const RenderFrustum& frustum, bool enabled);

    // Create, refresh, or remove render-side light proxies for one logical object.
    void syncLightProxy(GObject& object);

    // Remove all light proxies owned by this logical object.
    void removeLightProxy(GObjectID object_id);

    // Clear all render-side light proxies and cached light snapshots.
    void clearLightProxies();

    // Mirror object visibility to any light proxy owned by this object.
    void setLightVisible(GObjectID object_id, bool visible);

    // Update cached point-light position and matrices when its transform changes.
    void updateLightTransform(GObjectID object_id, const TransformComponent& transform);

    // Rebuild pass-facing light data from dirty render-side light proxies.
    void rebuildLightListsAndData();

    const std::vector<RenderMeshSection*>& visibleMeshSections() const { return m_visible_sections; }
    const std::vector<RenderMeshSection*>& opaqueMeshSections() const { return m_opaque_sections; }
    const std::vector<RenderMeshSection*>& transparentMeshSections() const { return m_transparent_sections; }
    const std::vector<RenderMeshSection*>& mainCameraVisibleMeshSections() const { return m_main_camera_culling_enabled ? m_main_camera_visible_sections : m_visible_sections; }
    const std::vector<RenderMeshSection*>& mainCameraOpaqueMeshSections() const { return m_main_camera_culling_enabled ? m_main_camera_opaque_sections : m_opaque_sections; }
    const std::vector<RenderMeshSection*>& mainCameraTransparentMeshSections() const { return m_main_camera_culling_enabled ? m_main_camera_transparent_sections : m_transparent_sections; }
    const std::vector<RenderMeshSection*>& skinnedMeshSections() const { return m_skinned_sections; }
    const std::vector<RenderMeshSection*>& staticShadowCasterSections() const { return m_static_shadow_caster_sections; }
    const std::vector<RenderMeshSection*>& dynamicShadowCasterSections() const { return m_dynamic_shadow_caster_sections; }

    uint64_t shadowStaticVersion() const { return m_shadow_static_version; }

    const std::vector<RenderDirectionalLightData>& directionalLightData() const { return m_cached_directional_light_data; }
    const std::vector<RenderPointLightData>& pointLightData() const { return m_cached_point_light_data; }
    const std::vector<RenderPointLightInstanceData>& pointLightInstanceData() const { return m_point_light_instance_data; }
    bool pointLightInstanceDataDirty() const { return m_point_light_instance_data_dirty; }
    void clearPointLightInstanceDataDirty() { m_point_light_instance_data_dirty = false; }

    bool hasTransparent() const { return m_has_transparent; }
    bool mainCameraHasTransparent() const { return m_main_camera_culling_enabled ? m_main_camera_has_transparent : m_has_transparent; }

    RenderSkybox& skybox() { return m_skybox; }
    const RenderSkybox& skybox() const { return m_skybox; }

    void clearObjectProxies();
    void clear();

private:
    std::unordered_map<GObjectID, std::unique_ptr<RenderObjectProxy>> m_object_proxies;
    std::vector<RenderMeshSection*> m_visible_sections;
    std::vector<RenderMeshSection*> m_opaque_sections;
    std::vector<RenderMeshSection*> m_transparent_sections;
    std::vector<RenderMeshSection*> m_main_camera_visible_sections;
    std::vector<RenderMeshSection*> m_main_camera_opaque_sections;
    std::vector<RenderMeshSection*> m_main_camera_transparent_sections;
    std::vector<RenderMeshSection*> m_skinned_sections;
    std::vector<RenderMeshSection*> m_static_shadow_caster_sections;
    std::vector<RenderMeshSection*> m_dynamic_shadow_caster_sections;
    uint64_t m_shadow_static_version{ 1 };
    bool m_has_transparent{ false };
    bool m_main_camera_has_transparent{ false };
    bool m_main_camera_culling_enabled{ false };

    std::unordered_map<GObjectID, RenderDirectionalLightProxy> m_directional_light_proxies;
    std::unordered_map<GObjectID, RenderPointLightProxy> m_point_light_proxies;
    std::vector<RenderDirectionalLightProxy*> m_visible_directional_lights;
    std::vector<RenderPointLightProxy*> m_visible_point_lights;
    std::vector<RenderDirectionalLightData> m_cached_directional_light_data;
    std::vector<RenderPointLightData> m_cached_point_light_data;
    std::vector<RenderPointLightInstanceData> m_point_light_instance_data;
    bool m_light_lists_dirty{ true };
    bool m_point_light_instance_data_dirty{ true };

    RenderSkybox m_skybox;
};

#endif // !RenderScene_hpp
