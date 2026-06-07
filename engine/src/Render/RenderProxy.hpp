#ifndef RenderProxy_hpp
#define RenderProxy_hpp

#include "Base/Common.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "Render/RenderPrimitive.hpp"
#include "Render/RHI/rhi.hpp"

#include <array>
#include <memory>
#include <vector>

// Scene environment skybox data used by the skybox pass.
struct RenderSkybox {
    RhiTexture* skyboxCubeMap() const { return external_skybox_cube_map ? external_skybox_cube_map : (skybox_cube_map ? skybox_cube_map->texture() : nullptr); }

    std::shared_ptr<RenderTextureResource> skybox_cube_map;
    RhiTexture* external_skybox_cube_map{ nullptr };
    std::shared_ptr<RenderMeshResource> mesh;
};

// Render-side directional light proxy. One logical light object owns one proxy.
struct RenderDirectionalLightProxy {
    GObjectID object_id;
    bool visible{ true };
    Color3 color{ 1.0f };
    Vec3 direction{ 0.0f, -1.0f, 0.0f };
    float aspect_ratio{ 16.0f / 9.0f };
    Mat4 light_view_matrix{ 1.0f };
    Mat4 light_proj_matrix{ 1.0f };
};

// Render-side point light proxy. It caches CPU-side data used by lighting and debug draw passes.
struct RenderPointLightProxy {
    GObjectID object_id;
    bool visible{ true };
    Color3 color{ 1.0f };
    Vec3 position{ 0.0f };
    float radius{ 30.0f };
    bool cast_shadow{ true };
    std::array<Mat4, 6> light_view_matrices{};
    Mat4 light_proj_matrix{ 1.0f };
};

struct RenderPointLightInstanceData {
    Mat4 inst_matrix{ 1.0f };
    Color3 inst_color{ 1.0f };
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

#endif // !RenderProxy_hpp
