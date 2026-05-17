#ifndef RenderSourceData_hpp
#define RenderSourceData_hpp

#include "Base/Common.hpp"
#include "Logical/Texture.hpp"
#include "Logical/Mesh.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "RenderShaderObject.hpp"
#include "Render/RHI/rhi.hpp"

using GL_RESOURCE_HANLE = unsigned int;

struct RenderTextureData {
    RenderTextureData(std::shared_ptr<Texture> texture_);
    RenderTextureData(const CubeTexture& cube_texture_);

    GL_RESOURCE_HANLE id;

    static RenderTextureData& defaultTexture();
};

struct RenderMaterialData {
    RenderMaterialData(std::shared_ptr<Material> material_);

    GL_RESOURCE_HANLE albedo_map{ 0 };
    GL_RESOURCE_HANLE metallic_map{ 0 };
    GL_RESOURCE_HANLE roughness_map{ 0 };
    GL_RESOURCE_HANLE ao_map{ 0 };
    Vec3 base_color_factor{ 1.0f, 1.0f, 1.0f };
    float metallic_factor{ 0.0f };
    float roughness_factor{ 1.0f };
    float ao_factor{ 1.0f };

    GL_RESOURCE_HANLE diffuse_map{ 0 };
    GL_RESOURCE_HANLE specular_map{ 0 };
    GL_RESOURCE_HANLE normal_map{ 0 };
    GL_RESOURCE_HANLE height_map{ 0 };
    Vec3 diffuse_factor{ 1.0f, 1.0f, 1.0f };
    Vec3 specular_factor{ 1.0f, 1.0f, 1.0f };
    float shininess{ 128.0f };

    float alpha{ 1.0f };
    uint64_t material_version{ 0 };
};

class RenderMeshData {
public:
    RenderMeshData(std::shared_ptr<Mesh> mesh_data);
    ~RenderMeshData() { reset(); }

    void reset();
    GL_RESOURCE_HANLE getVAO() const { return m_vertex_layout->id(); }
    size_t verticesCount() const { return m_vertices_count; }
    size_t indicesCount() const { return m_indices_count; }
    void create_instancing(void* instancing_data, int instancing_data_size, int buffer_capacity_size = -1);
    void update_instancing(void* instancing_data, int instancing_data_size);
    int instancingCapacityBytes() const { return m_instancing_capacity_bytes; }

private:
    RhiVertexLayout* m_vertex_layout{ nullptr };
    size_t m_vertices_count;
    size_t m_indices_count;

    RhiBuffer* m_instancing_buffer{ nullptr };
    int m_instancing_capacity_bytes{ 0 };
};

class RenderMeshInstanceData {
    // TODO
};

struct RenderDirectionalLightData {
    Color3 color;
    Vec3 direction;
    Mat4 lightViewMatrix;
    Mat4 lightProjMatrix;
};

struct RenderPointLightData {
    int id;
    Color3 color;
    Vec3 position;
    float radius;
    std::array<Mat4, 6> lightViewMatrix;
    Mat4 lightProjMatrix;
};


struct RenderMeshNodeID {
    RenderMeshNodeID(GObjectID object_id, int sub_mesh_idx)
        : object_id(object_id)
        , sub_mesh_idx(sub_mesh_idx)
    {}
    bool operator==(const RenderMeshNodeID& rhs) const {
        return object_id == rhs.object_id && sub_mesh_idx == rhs.sub_mesh_idx;
    }
    GObjectID object_id;
    int sub_mesh_idx;
};
struct RenderMeshNodeIDHasher {
    size_t operator()(const RenderMeshNodeID& id) const {
        return (std::hash<int>()(id.object_id.id) ^ (std::hash<int>()(id.sub_mesh_idx)) << 1);
    }
};

struct RenderMeshNode {
    RenderMeshNode(
        const RenderMeshNodeID& id,
        const RenderMeshData& mesh_data,
        const RenderMaterialData& material_data,
        Mat4 matrix,
        int source_index_offset_,
        int source_index_count_)
        : node_id(id)
        , mesh(mesh_data)
        , material(material_data)
        , model_matrix(matrix)
        , source_index_offset(source_index_offset_)
        , source_index_count(source_index_count_)
    {}

    RenderMeshNodeID node_id;
    RenderMeshData mesh;
    RenderMaterialData material;
    Mat4 model_matrix;
    int source_index_offset{ 0 };
    int source_index_count{ 0 };
    bool use_skinning{ false };
    std::vector<Mat4> bone_matrices;

    void updateRenderMaterialData(std::shared_ptr<Material> material_);
};

struct RenderSkyboxNode {
    GL_RESOURCE_HANLE skybox_cube_map;

    std::shared_ptr<RenderMeshData> mesh;
};

struct RenderCameraData {
    float fov;
    Vec3 pos;
    Vec3 direction;
    Vec3 rightDirection;
    Vec3 upDirection;
};

struct RenderSourceData {
    static inline std::shared_ptr<Rhi> rhi;
    static void initRHI() {
        // TODO 写成单例？
        rhi = std::shared_ptr<Rhi>(Rhi::create());
    }

    std::unordered_map<RenderMeshNodeID, std::shared_ptr<RenderMeshNode>, RenderMeshNodeIDHasher> render_mesh_nodes;
    std::vector<RenderDirectionalLightData> render_directional_light_data_list;
    std::vector<RenderPointLightData> render_point_light_data_list;
    std::shared_ptr<RenderMeshData> render_point_light_inst_mesh;
    int point_light_inst_amount{ 0 };
    RenderSkyboxNode render_skybox_node;
    std::shared_ptr<RenderMeshData> screen_quad;

    std::vector<GObjectID> picked_ids;

    Vec3 camera_position;
    Mat4 view_matrix;
    Mat4 proj_matrix;

    std::shared_ptr<RenderCameraData> render_camera;

    void reset() {
        render_mesh_nodes.clear();
        render_directional_light_data_list.clear();
        render_point_light_data_list.clear();
        render_skybox_node.mesh.reset();
        picked_ids.clear();
    }
};

#endif // !RenderSourceData_hpp
