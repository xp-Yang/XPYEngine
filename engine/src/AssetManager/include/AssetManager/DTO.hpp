#ifndef DTO_hpp
#define DTO_hpp

#include "Base/Common.hpp"
#include "Base/Meta/Serializer.hpp"

#include <string>
#include <vector>

struct TransformDTO
{
	Vec3 translation{0.0f, 0.0f, 0.0f};
	Vec3 rotation{0.0f, 0.0f, 0.0f};
	Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct MaterialTexturesDTO
{
	std::string diffuse;
	std::string specular;
	std::string normal;
	std::string height;
	std::string albedo;
	std::string metallic;
	std::string roughness;
	std::string ao;
};

struct MaterialDTO
{
	float alpha{1.0f};
	Vec3 base_color_factor{1.0f, 1.0f, 1.0f};
	float metallic_factor{0.0f};
	float roughness_factor{1.0f};
	float ao_factor{1.0f};
	Vec3 diffuse_factor{1.0f, 1.0f, 1.0f};
	Vec3 specular_factor{1.0f, 1.0f, 1.0f};
	float shininess{128.0f};
	MaterialTexturesDTO textures;
};

enum class FileType : int
{
	None = 0,
	OBJ,
	STL,
	JSON,
	CustomCube,
	CustomSphere,
	CustomGround,
	CustomScreen,
};

struct SubMeshDTO
{
	int sub_mesh_index{0};
	TransformDTO local_transform{};
};

struct PointLightDTO
{
	Color3 luminous_color{1.0f, 1.0f, 1.0f};
	float radius{30.0f};
	bool cast_shadow{true};
};

struct DirectionalLightDTO
{
	Color3 luminous_color{1.0f, 1.0f, 1.0f};
	Vec3 direction{15.0f, -30.0f, 15.0f};
	float aspect_ratio{16.0f / 9.0f};
};

struct CameraDTO
{
	int mode{0};
	int projection_mode{0};
	int zoom_mode{0};
	float origin_fov{Math::deg2rad(45.0f)};
	float fov{Math::deg2rad(45.0f)};
	float near_plane{0.1f};
	float far_plane{1000.0f};
	float aspect_ratio{16.0f / 9.0f};
	Vec3 position{0.0f, 30.0f, 30.0f};
	Vec3 direction{Math::Normalize(Vec3(0.0f, -1.0f, -1.0f))};
	Vec3 up_direction{Math::Normalize(Vec3(0.0f, 1.0f, 0.0f) - Math::Dot(Vec3(0.0f, 1.0f, 0.0f), direction) * direction)};
};

struct ObjectDTO
{
	std::string name;
	bool visible{true};
	TransformDTO transform;
	std::string filepath;
	int file_type{static_cast<int>(FileType::OBJ)};
	bool static_shadow_caster{true};
	std::vector<SubMeshDTO> sub_meshes;
	std::vector<MaterialDTO> materials;
	bool has_point_light{false};
	PointLightDTO point_light;
	bool has_directional_light{false};
	DirectionalLightDTO directional_light;
	bool has_camera{false};
	CameraDTO camera;
};

struct ProjectDTO
{
	std::string project_name{"XPYProject"};
	std::vector<ObjectDTO> objects;
};

#endif // !DTO_hpp
