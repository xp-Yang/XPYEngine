#include "AssetManager/Mesh.hpp"

#include "Base/Logger/Logger.hpp"

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<int> &indices)
    : vertices(vertices), indices(indices)
{
    index_count = static_cast<int>(indices.size());
}

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<int> &indices, std::shared_ptr<Material> material_)
    : vertices(vertices), indices(indices), material(material_)
{
    index_count = static_cast<int>(indices.size());
}

void Mesh::reset()
{
    sub_mesh_idx = 0;
    index_offset = 0;
    index_count = 0;
    vertices.clear();
    indices.clear();
    vertices.shrink_to_fit();
    indices.shrink_to_fit();
    material.reset();
    translation = Vec3(0.0f);
    rotation = Vec3(0.0f);
    scale = Vec3(1.0f);
}
