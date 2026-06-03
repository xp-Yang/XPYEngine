#include "AssetManager/Mesh.hpp"

#include "Base/Logger/Logger.hpp"

MeshGeometry::MeshGeometry(const std::vector<Vertex> &vertices_, const std::vector<int> &indices_)
    : vertices(vertices_), indices(indices_)
{
}

void MeshGeometry::reset()
{
    vertices.clear();
    indices.clear();
    vertices.shrink_to_fit();
    indices.shrink_to_fit();
}

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<int> &indices)
    : Mesh(std::make_shared<MeshGeometry>(vertices, indices))
{
}

Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<int> &indices, std::shared_ptr<Material> material_)
    : Mesh(std::make_shared<MeshGeometry>(vertices, indices), material_)
{
}

Mesh::Mesh(std::shared_ptr<MeshGeometry> geometry_)
    : geometry(geometry_ ? geometry_ : std::make_shared<MeshGeometry>())
{
    index_count = static_cast<int>(geometry->indices.size());
}

Mesh::Mesh(std::shared_ptr<MeshGeometry> geometry_, std::shared_ptr<Material> material_)
    : Mesh(geometry_)
{
    material = material_;
}

void Mesh::reset()
{
    sub_mesh_idx = 0;
    index_offset = 0;
    index_count = 0;
    geometry = std::make_shared<MeshGeometry>();
    material.reset();
    translation = Vec3(0.0f);
    rotation = Vec3(0.0f);
    scale = Vec3(1.0f);
}
