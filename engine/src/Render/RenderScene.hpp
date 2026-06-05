#ifndef RenderScene_hpp
#define RenderScene_hpp

#include "Base/Common.hpp"
#include "AssetManager/Texture.hpp"
#include "AssetManager/Mesh.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "Render/RenderCulling.hpp"
#include "Render/RHI/rhi.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct RenderTextureResource {
    explicit RenderTextureResource(std::shared_ptr<Texture> texture_);
    explicit RenderTextureResource(std::shared_ptr<CubeTexture> cube_texture_);
    ~RenderTextureResource();

    RenderTextureResource(const RenderTextureResource&) = delete;
    RenderTextureResource& operator=(const RenderTextureResource&) = delete;
    RenderTextureResource(RenderTextureResource&&) = delete;
    RenderTextureResource& operator=(RenderTextureResource&&) = delete;

    RhiTexture* texture() const { return m_texture; }
    GL_HANDLE id() const { return m_texture ? m_texture->id() : 0; }

    static std::shared_ptr<RenderTextureResource> textureOf(std::shared_ptr<Texture> texture_);
    static std::shared_ptr<RenderTextureResource> cubeTextureOf(std::shared_ptr<CubeTexture> cube_texture_);
    static std::shared_ptr<RenderTextureResource> defaultTexture();
    static std::shared_ptr<RenderTextureResource> defaultCubeTexture();

private:
    std::shared_ptr<Texture> m_source_texture;
    std::shared_ptr<CubeTexture> m_source_cube_texture;
    RhiTexture* m_texture{ nullptr };
};

// Render-side material resource. It stores only GPU-ready material inputs.
class RenderMaterialResource {
public:
    explicit RenderMaterialResource(std::shared_ptr<Material> material_);

    // Refresh scalar factors and lazily fill missing texture handles from the source material.
    void updateFrom(std::shared_ptr<Material> material_);

    bool isTransparent() const { return alpha != 1.0f; }

    // PBR inputs.
    RhiTexture* albedoMap() const { return rawTextureOf(m_albedo_map); }
    RhiTexture* metallicMap() const { return rawTextureOf(m_metallic_map); }
    RhiTexture* roughnessMap() const { return rawTextureOf(m_roughness_map); }
    RhiTexture* aoMap() const { return rawTextureOf(m_ao_map); }
    Vec3 base_color_factor{ 1.0f, 1.0f, 1.0f };
    float metallic_factor{ 0.0f };
    float roughness_factor{ 1.0f };
    float ao_factor{ 1.0f };

    // Non-PBR / legacy shading inputs.
    RhiTexture* diffuseMap() const { return rawTextureOf(m_diffuse_map); }
    RhiTexture* specularMap() const { return rawTextureOf(m_specular_map); }
    RhiTexture* normalMap() const { return rawTextureOf(m_normal_map); }
    RhiTexture* heightMap() const { return rawTextureOf(m_height_map); }
    Vec3 diffuse_factor{ 1.0f, 1.0f, 1.0f };
    Vec3 specular_factor{ 1.0f, 1.0f, 1.0f };
    float shininess{ 128.0f };

    // 是否拥有真实法线贴图（normal_map 永不为空，会回退到默认贴图，需此标志门控）。
    bool has_normal_map{ false };

    // Shared material state.
    float alpha{ 1.0f };
    uint64_t material_version{ 0 };

private:
    static RhiTexture* rawTextureOf(const std::shared_ptr<RenderTextureResource>& texture_data)
    {
        return texture_data ? texture_data->texture() : nullptr;
    }

    std::shared_ptr<RenderTextureResource> m_albedo_map;
    std::shared_ptr<RenderTextureResource> m_metallic_map;
    std::shared_ptr<RenderTextureResource> m_roughness_map;
    std::shared_ptr<RenderTextureResource> m_ao_map;
    std::shared_ptr<RenderTextureResource> m_diffuse_map;
    std::shared_ptr<RenderTextureResource> m_specular_map;
    std::shared_ptr<RenderTextureResource> m_normal_map;
    std::shared_ptr<RenderTextureResource> m_height_map;
};

struct RenderGeometryGpuResource
{
    ~RenderGeometryGpuResource()
    {
        if (vertex_buffer)
        {
            vertex_buffer->destroy();
            delete vertex_buffer;
            vertex_buffer = nullptr;
        }
        if (index_buffer)
        {
            index_buffer->destroy();
            delete index_buffer;
            index_buffer = nullptr;
        }
    }

    RhiBuffer* vertex_buffer{ nullptr };
    RhiBuffer* index_buffer{ nullptr };
    size_t vertices_count{ 0 };
    size_t indices_count{ 0 };
};

// Render-side mesh resource that owns the RHI vertex/index layout for a mesh.
class RenderMeshResource {
public:
    RenderMeshResource(std::shared_ptr<Mesh> mesh_data);
    ~RenderMeshResource() { reset(); }
    RenderMeshResource(const RenderMeshResource&) = delete;
    RenderMeshResource& operator=(const RenderMeshResource&) = delete;
    RenderMeshResource(RenderMeshResource&& other) noexcept;
    RenderMeshResource& operator=(RenderMeshResource&& other) noexcept;

    void reset();
    GL_HANDLE getVAO() const { return m_vertex_layout ? m_vertex_layout->id() : 0; }
    RhiVertexLayout* vertexLayout() const { return m_vertex_layout; }
    size_t verticesCount() const { return m_vertices_count; }
    size_t indicesCount() const { return m_indices_count; }
    void create_instancing(void* instancing_data, int instancing_data_size, int buffer_capacity_size = -1);
    void update_instancing(void* instancing_data, int instancing_data_size);
    int instancingCapacityBytes() const { return m_instancing_capacity_bytes; }

private:
    std::shared_ptr<RenderGeometryGpuResource> m_geometry_resource;
    RhiVertexLayout* m_vertex_layout{ nullptr };
    size_t m_vertices_count{ 0 };
    size_t m_indices_count{ 0 };

    RhiBuffer* m_instancing_buffer{ nullptr };
    int m_instancing_capacity_bytes{ 0 };
};

class RenderMeshInstanceData {
    // TODO
};

struct RenderMeshSectionID {
    RenderMeshSectionID(GObjectID object_id, int sub_mesh_idx)
        : object_id(object_id)
        , sub_mesh_idx(sub_mesh_idx)
    {}
    bool operator==(const RenderMeshSectionID& rhs) const {
        return object_id == rhs.object_id && sub_mesh_idx == rhs.sub_mesh_idx;
    }
    GObjectID object_id;
    int sub_mesh_idx;
};
struct RenderMeshSectionIDHasher {
    size_t operator()(const RenderMeshSectionID& id) const {
        return (std::hash<GObjectID>()(id.object_id) ^ (std::hash<int>()(id.sub_mesh_idx)) << 1);
    }
};

class RenderObjectProxy;

// SubMesh/material-section render proxy. One section maps to one draw-range candidate.
struct RenderMeshSection {
    RenderMeshSection(
        const RenderMeshSectionID& id,
        RenderMeshResource&& mesh_data,
        const RenderMaterialResource& material_data,
        Mat4 matrix,
        const RenderAABB& local_bounds,
        int source_index_offset_,
        int source_index_count_);

    // Pull latest material values from the logical material.
    void updateRenderMaterial(std::shared_ptr<Material> material_);

    RenderMeshSectionID section_id;
    RenderObjectProxy* owner{ nullptr };
    RenderMeshResource mesh;
    RenderMaterialResource material;
    Mat4 local_matrix{ 1.0f };
    Mat4 model_matrix;
    RenderAABB local_bounds;
    RenderAABB world_bounds;
    int source_index_offset{ 0 };
    int source_index_count{ 0 };
    bool visible{ true };
    bool use_skinning{ false };
    bool static_shadow_caster{ true };
    std::vector<Mat4> bone_matrices;
};

// Scene environment skybox data used by the skybox pass.
struct RenderSkybox {
    RhiTexture* skyboxCubeMap() const { return external_skybox_cube_map ? external_skybox_cube_map : (skybox_cube_map ? skybox_cube_map->texture() : nullptr); }

    std::shared_ptr<RenderTextureResource> skybox_cube_map;
    RhiTexture* external_skybox_cube_map{ nullptr };
    std::shared_ptr<RenderMeshResource> mesh;
};

// Object-level render proxy. One logical GObject owns zero or more mesh sections.
class RenderObjectProxy {
public:
    explicit RenderObjectProxy(GObjectID object_id);

    GObjectID objectID() const { return m_object_id; }
    bool visible() const { return m_visible; }

    // Update object-level visibility and mirror it to all owned sections.
    void setVisible(bool visible);

    // Store the object transform used to rebuild section model matrices.
    void setModelMatrix(const Mat4& model_matrix);
    const Mat4& modelMatrix() const { return m_model_matrix; }

    // Add a section owned by this proxy and return the stable raw pointer.
    RenderMeshSection* addMeshSection(std::unique_ptr<RenderMeshSection> section);

    // Find a section by subMesh index inside this object.
    RenderMeshSection* meshSection(int sub_mesh_idx);
    const RenderMeshSection* meshSection(int sub_mesh_idx) const;

    bool hasVisibleStaticShadowCaster() const;

    std::vector<std::unique_ptr<RenderMeshSection>>& meshSections() { return m_mesh_sections; }
    const std::vector<std::unique_ptr<RenderMeshSection>>& meshSections() const { return m_mesh_sections; }

private:
    GObjectID m_object_id;
    bool m_visible{ true };
    Mat4 m_model_matrix{ 1.0f };
    std::vector<std::unique_ptr<RenderMeshSection>> m_mesh_sections;
};

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
    RenderSkybox m_skybox;
};

#endif // !RenderScene_hpp
