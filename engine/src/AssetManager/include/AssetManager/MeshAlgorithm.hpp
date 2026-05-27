#ifndef MeshAlgorithm_hpp
#define MeshAlgorithm_hpp

#include "AssetManager/Mesh.hpp"

namespace MeshAlgorithm {

std::shared_ptr<Mesh> create_cube_mesh();
std::shared_ptr<Mesh> create_cuboid_mesh(const std::array<Vec3, 8> vertex_positions);
std::shared_ptr<Mesh> create_icosphere_mesh(float radius, int regression_depth);
std::shared_ptr<Mesh> create_quad_mesh(const Point3 &origin, const Vec3 &positive_dir_u, const Vec3 &positive_dir_v);
std::shared_ptr<Mesh> create_complex_quad_mesh(const Vec2 &size);
std::shared_ptr<Mesh> create_screen_mesh();

}

#endif
