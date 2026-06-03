#ifndef Mesh_hpp
#define Mesh_hpp

#include <Base/Common.hpp>
#include "Material.hpp"

#define MAX_BONE_INFLUENCE 4
#define MAX_BONE_PALETTE_SIZE 100
struct Vertex {
	Vec3 position;
	Vec3 normal;
	Vec2 texture_uv;
	Vec4 tangent{ 0.0f, 0.0f, 0.0f, 1.0f }; // xyz = 切线，w = 副切线手性（应对镜像 UV）
	std::array<int, MAX_BONE_INFLUENCE> bone_ids{ -1, -1, -1, -1 };
	std::array<float, MAX_BONE_INFLUENCE> bone_weights{ 0.0f, 0.0f, 0.0f, 0.0f };
};

struct Triangle {
	Triangle(const Vec3& pos1, const Vec3& pos2, const Vec3& pos3) {
		vertices[0].position = pos1;
		vertices[1].position = pos2;
		vertices[2].position = pos3;
	}
	std::array<Vertex, 3> vertices;
};

struct MeshGeometry {
	MeshGeometry() = default;
	MeshGeometry(const std::vector<Vertex>& vertices_, const std::vector<int>& indices_);

	void reset();

	std::vector<Vertex> vertices;
	std::vector<int> indices;
};

struct Mesh {
	Mesh() = delete;
	Mesh(const std::vector<Vertex>& vertices, const std::vector<int>& indices);
	Mesh(const std::vector<Vertex>& vertices, const std::vector<int>& indices, std::shared_ptr<Material> material_);
	Mesh(std::shared_ptr<MeshGeometry> geometry_);
	Mesh(std::shared_ptr<MeshGeometry> geometry_, std::shared_ptr<Material> material_);
	//Mesh(const std::vector<Triangle>& triangles);

	void reset();

	const std::vector<Vertex>& vertices() const { return geometry->vertices; }
	const std::vector<int>& indices() const { return geometry->indices; }
	std::vector<Vertex>& mutableVertices() { return geometry->vertices; }
	std::vector<int>& mutableIndices() { return geometry->indices; }

	int sub_mesh_idx{ 0 };
	int index_offset{ 0 };
	int index_count{ 0 };
	std::shared_ptr<MeshGeometry> geometry;
	std::shared_ptr<Material> material;
	Vec3 translation{ 0.0f, 0.0f, 0.0f };
	Vec3 rotation{ 0.0f, 0.0f, 0.0f };
	Vec3 scale{ 1.0f, 1.0f, 1.0f };
};

#endif
