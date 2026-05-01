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

struct Mesh {
	static std::shared_ptr<Mesh> create_cube_mesh();
	static std::shared_ptr<Mesh> create_cuboid_mesh(const std::array<Vec3, 8> vertex_positions);
	static std::shared_ptr<Mesh> create_icosphere_mesh(float radius, int regression_depth);
	static std::shared_ptr<Mesh> create_quad_mesh(const Point3& origin, const Vec3& positive_dir_u, const Vec3& positive_dir_v);
	static std::shared_ptr<Mesh> create_complex_quad_mesh(const Vec2& size);
	static std::shared_ptr<Mesh> create_screen_mesh();

	Mesh() = delete;
	Mesh(const std::vector<Vertex>& vertices, const std::vector<int>& indices);
	Mesh(const std::vector<Vertex>& vertices, const std::vector<int>& indices, std::shared_ptr<Material> material_);
	//Mesh(const std::vector<Triangle>& triangles);

	void reset();

	int sub_mesh_idx{ 0 };
	std::vector<Vertex> vertices;
	std::vector<int> indices;
	std::shared_ptr<Material> material;
	Vec3 translation{ 0.0f, 0.0f, 0.0f };
	Vec3 rotation{ 0.0f, 0.0f, 0.0f };
	Vec3 scale{ 1.0f, 1.0f, 1.0f };
};

#endif
