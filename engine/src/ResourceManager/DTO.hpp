#ifndef DTO_hpp
#define DTO_hpp

#include "Base/Common.hpp"
#include "Base/Meta/Serializer.hpp"

#include <string>
#include <vector>


struct TransformDTO {
	Vec3 translation{ 0.0f, 0.0f, 0.0f };
	Vec3 rotation{ 0.0f, 0.0f, 0.0f };
	Vec3 scale{ 1.0f, 1.0f, 1.0f };
};

struct MaterialTexturesDTO {
	std::string diffuse;
	std::string specular;
	std::string normal;
	std::string height;
	std::string albedo;
	std::string metallic;
	std::string roughness;
	std::string ao;
};

struct MaterialDTO {
	float alpha{ 1.0f };
	Vec3 base_color_factor{ 1.0f, 1.0f, 1.0f };
	float metallic_factor{ 0.0f };
	float roughness_factor{ 1.0f };
	float ao_factor{ 1.0f };
	Vec3 diffuse_factor{ 1.0f, 1.0f, 1.0f };
	Vec3 specular_factor{ 1.0f, 1.0f, 1.0f };
	float shininess{ 128.0f };
	MaterialTexturesDTO textures;
};

enum class FileType : int {
	None = 0,
	OBJ,
	STL,
	JSON,
	CustomCube,
	CustomSphere,
	CustomGround,
	CustomScreen,
};

struct SubMeshDTO {
	int sub_mesh_index{ 0 };
	TransformDTO local_transform{};
};

struct ObjectDTO {
	std::string name;
	bool visible{ true };
	TransformDTO transform;
	std::string filepath;
	int file_type{ static_cast<int>(FileType::OBJ) };
	std::vector<SubMeshDTO> sub_meshes;
	std::vector<MaterialDTO> materials;
};

struct ProjectDTO {
	int schema_version{ 3 };
	std::string project_name{ "XPYProject" };
	std::vector<ObjectDTO> objects;
};

#endif // !DTO_hpp
