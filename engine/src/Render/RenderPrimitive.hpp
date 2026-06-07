#ifndef RenderPrimitive_hpp
#define RenderPrimitive_hpp

#include "Base/Common.hpp"
#include "AssetManager/Mesh.hpp"
#include "AssetManager/Texture.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "Render/RenderCulling.hpp"
#include "Render/RHI/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class RenderObjectProxy;

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
    ~RenderGeometryGpuResource();

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

// Transform a local render bound into world space for CPU-side culling and shadow list maintenance.
RenderAABB transformRenderAABB(const RenderAABB& bounds, const Mat4& matrix);

// SubMesh/material-section render primitive. One section maps to one draw-range candidate.
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

#endif // !RenderPrimitive_hpp
