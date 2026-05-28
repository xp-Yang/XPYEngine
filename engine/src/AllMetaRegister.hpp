#ifndef AllMetaRegister_hpp
#define AllMetaRegister_hpp

#include "Meta.hpp"

#include "AssetManager/Mesh.hpp"

#if ENABLE_ECS
#include "Logical/Framework/ECS/Components.hpp"
#endif

#include "Logical/Framework/World/Scene.hpp"
#include "AssetManager/Shader.hpp"
#include "AssetManager/DTO.hpp"

namespace Meta {

namespace Registration {

inline void allMetaRegister()
{
	// Vec2
	registerClass<Vec2>("Vec2").
		registerProperty(&Vec2::x, "x").
		registerProperty(&Vec2::y, "y");

	// Vec3
	registerClass<Vec3>("Vec3").
		registerProperty(&Vec3::x, "x").
		registerProperty(&Vec3::y, "y").
		registerProperty(&Vec3::z, "z");

	// Vec4
	registerClass<Vec4>("Vec4").
		registerProperty(&Vec4::x, "x").
		registerProperty(&Vec4::y, "y").
		registerProperty(&Vec4::z, "z").
		registerProperty(&Vec4::w, "w");

    //// Color3
    //registerClass<Color3>("Color3").
    //    registerProperty(&Color3::x, "x").
    //    registerProperty(&Color3::y, "y").
    //    registerProperty(&Color3::z, "z");

    //// Color4
	//registerClass<Color4>("Color4").
	//	registerProperty(&Color4::x, "x").
	//	registerProperty(&Color4::y, "y").
	//	registerProperty(&Color4::z, "z").
	//	registerProperty(&Color4::w, "w");

	// Mat3
	registerClass<Mat3>("Mat3");

	// Mat4
	registerClass<Mat4>("Mat4");

	registerClass<Mesh>("Mesh").
		registerProperty(&Mesh::sub_mesh_idx, "sub_mesh_idx").
		registerProperty(&Mesh::vertices, "vertices").
		registerProperty(&Mesh::indices, "indices").
		registerProperty(&Mesh::material, "material").
		registerProperty(&Mesh::translation, "translation").
		registerProperty(&Mesh::rotation, "rotation").
		registerProperty(&Mesh::scale, "scale");
	registerClass<Texture>("Texture").
		registerProperty(&Texture::texture_type, "texture_type").
		registerProperty(&Texture::texture_filepath, "texture_filepath").
		registerProperty(&Texture::width, "width").
		registerProperty(&Texture::height, "height").
		registerProperty(&Texture::channel_count, "channel_count").
		registerProperty(&Texture::gamma, "gamma");
	registerClass<Material>("Material").
		registerProperty(&Material::albedo_texture, "albedo_texture").
		registerProperty(&Material::metallic_texture, "metallic_texture").
		registerProperty(&Material::roughness_texture, "roughness_texture").
		registerProperty(&Material::ao_texture, "ao_texture").
		registerProperty(&Material::base_color_factor, "base_color_factor").
		registerProperty(&Material::metallic_factor, "metallic_factor").
		registerProperty(&Material::roughness_factor, "roughness_factor").
		registerProperty(&Material::ao_factor, "ao_factor").
		registerProperty(&Material::diffuse_texture, "diffuse_texture").
		registerProperty(&Material::specular_texture, "specular_texture").
		registerProperty(&Material::normal_texture, "normal_texture").
		registerProperty(&Material::height_texture, "height_texture").
		registerProperty(&Material::diffuse_factor, "diffuse_factor").
		registerProperty(&Material::specular_factor, "specular_factor").
		registerProperty(&Material::shininess, "shininess").
		registerProperty(&Material::alpha, "alpha");
	registerClass<Shader>("Shader");
	registerClass<CubeTexture>("CubeTexture");

	// DTOs
	registerClass<TransformDTO>("TransformDTO")
		.registerProperty(&TransformDTO::translation, "translation")
		.registerProperty(&TransformDTO::rotation, "rotation")
		.registerProperty(&TransformDTO::scale, "scale");
	registerClass<MaterialTexturesDTO>("MaterialTexturesDTO")
		.registerProperty(&MaterialTexturesDTO::diffuse, "diffuse")
		.registerProperty(&MaterialTexturesDTO::specular, "specular")
		.registerProperty(&MaterialTexturesDTO::normal, "normal")
		.registerProperty(&MaterialTexturesDTO::height, "height")
		.registerProperty(&MaterialTexturesDTO::albedo, "albedo")
		.registerProperty(&MaterialTexturesDTO::metallic, "metallic")
		.registerProperty(&MaterialTexturesDTO::roughness, "roughness")
		.registerProperty(&MaterialTexturesDTO::ao, "ao");
	registerClass<MaterialDTO>("MaterialDTO")
		.registerProperty(&MaterialDTO::alpha, "alpha")
		.registerProperty(&MaterialDTO::base_color_factor, "base_color_factor")
		.registerProperty(&MaterialDTO::metallic_factor, "metallic_factor")
		.registerProperty(&MaterialDTO::roughness_factor, "roughness_factor")
		.registerProperty(&MaterialDTO::ao_factor, "ao_factor")
		.registerProperty(&MaterialDTO::diffuse_factor, "diffuse_factor")
		.registerProperty(&MaterialDTO::specular_factor, "specular_factor")
		.registerProperty(&MaterialDTO::shininess, "shininess")
		.registerProperty(&MaterialDTO::textures, "textures");
	registerClass<SubMeshDTO>("SubMeshDTO")
		.registerProperty(&SubMeshDTO::sub_mesh_index, "index")
		.registerProperty(&SubMeshDTO::local_transform, "local_transform");
	registerClass<PointLightDTO>("PointLightDTO")
		.registerProperty(&PointLightDTO::luminous_color, "luminous_color")
		.registerProperty(&PointLightDTO::radius, "radius");
	registerClass<DirectionalLightDTO>("DirectionalLightDTO")
		.registerProperty(&DirectionalLightDTO::luminous_color, "luminous_color")
		.registerProperty(&DirectionalLightDTO::direction, "direction")
		.registerProperty(&DirectionalLightDTO::aspect_ratio, "aspect_ratio");
	registerClass<CameraDTO>("CameraDTO")
		.registerProperty(&CameraDTO::mode, "mode")
		.registerProperty(&CameraDTO::projection_mode, "projection_mode")
		.registerProperty(&CameraDTO::zoom_mode, "zoom_mode")
		.registerProperty(&CameraDTO::origin_fov, "origin_fov")
		.registerProperty(&CameraDTO::fov, "fov")
		.registerProperty(&CameraDTO::near_plane, "near_plane")
		.registerProperty(&CameraDTO::far_plane, "far_plane")
		.registerProperty(&CameraDTO::aspect_ratio, "aspect_ratio")
		.registerProperty(&CameraDTO::position, "position")
		.registerProperty(&CameraDTO::direction, "direction")
		.registerProperty(&CameraDTO::up_direction, "up_direction");
	registerClass<ObjectDTO>("ObjectDTO")
		.registerProperty(&ObjectDTO::name, "name")
		.registerProperty(&ObjectDTO::visible, "visible")
		.registerProperty(&ObjectDTO::transform, "transform")
		.registerProperty(&ObjectDTO::filepath, "filepath")
		.registerProperty(&ObjectDTO::file_type, "file_type")
		.registerProperty(&ObjectDTO::sub_meshes, "sub_meshes")
		.registerProperty(&ObjectDTO::materials, "materials")
		.registerProperty(&ObjectDTO::has_point_light, "has_point_light")
		.registerProperty(&ObjectDTO::point_light, "point_light")
		.registerProperty(&ObjectDTO::has_directional_light, "has_directional_light")
		.registerProperty(&ObjectDTO::directional_light, "directional_light")
		.registerProperty(&ObjectDTO::has_camera, "has_camera")
		.registerProperty(&ObjectDTO::camera, "camera");
	registerClass<ProjectDTO>("ProjectDTO")
		.registerProperty(&ProjectDTO::project_name, "project_name")
		.registerProperty(&ProjectDTO::objects, "objects");

#if ENABLE_ECS
	// Name
	registerClass<ecs::NameComponent>();
	registerProperty(&ecs::NameComponent::name, "name");

	// Transform
	registerClass<ecs::TransformComponent>();
	registerProperty(&ecs::TransformComponent::translation, "translation");
	registerProperty(&ecs::TransformComponent::rotation, "rotation");
	registerProperty(&ecs::TransformComponent::scale, "scale");
	registerMethod(&ecs::TransformComponent::transform, "transform");

	// Entity
	registerClass<ecs::Entity>();
	registerProperty(&ecs::Entity::m_id, "m_id");
	//registerProperty(&ecs::Entity::m_mask, "m_mask");

	// ComponentPool
	registerClass<ecs::ComponentPool>();
	registerProperty(&ecs::ComponentPool::m_data, "m_data");
	registerProperty(&ecs::ComponentPool::m_componentTypeSize, "m_componentTypeSize", true);

	// World
	registerClass<ecs::World>();
	registerProperty(&ecs::World::m_entities, "m_entities", true);
	registerProperty(&ecs::World::m_component_pools, "m_component_pools", true);
#else
	registerClass<::GObjectID>("GObjectID").
		registerProperty(&::GObjectID::id, "id");

	registerClass<::Component>("Component");

	registerClass<::GObject>("GObject").
		registerProperty(&::GObject::m_id, "m_id").
		registerProperty(&::GObject::m_name, "m_name").
		registerProperty(&::GObject::m_components, "m_components");

	registerClass<::MeshComponent>("MeshComponent").
		registerProperty(&::MeshComponent::source_filepath, "source_filepath").
		registerProperty(&::MeshComponent::sub_meshes, "sub_meshes");

	registerClass<::TransformComponent>("TransformComponent").
		registerProperty(&::TransformComponent::translation, "translation").
		registerProperty(&::TransformComponent::rotation, "rotation").
		registerProperty(&::TransformComponent::scale, "scale");

	registerClass<::CameraComponent>("CameraComponent").
		registerProperty(&::CameraComponent::mode, "mode").
		registerProperty(&::CameraComponent::projection_mode, "projection_mode").
		registerProperty(&::CameraComponent::zoom_mode, "zoom_mode").
		registerProperty(&::CameraComponent::originFov, "originFov").
		registerProperty(&::CameraComponent::fov, "fov").
		registerProperty(&::CameraComponent::nearPlane, "nearPlane").
		registerProperty(&::CameraComponent::farPlane, "farPlane").
		registerProperty(&::CameraComponent::direction, "direction").
		registerProperty(&::CameraComponent::pos, "pos").
		registerProperty(&::CameraComponent::upDirection, "upDirection").
		registerProperty(&::CameraComponent::view, "view").
		registerProperty(&::CameraComponent::projection, "projection");

	registerClass<::AnimationComponent>("AnimationComponent")
		.registerProperty(&::AnimationComponent::clip_path, "clip_path")
		.registerProperty(&::AnimationComponent::speed, "speed")
		.registerProperty(&::AnimationComponent::loop, "loop")
		.registerProperty(&::AnimationComponent::playing, "playing");
	registerClass<::RigidComponent>("RigidComponent");

	registerClass<::LightComponent>("LightComponent")
		.registerProperty(&::LightComponent::luminousColor, "luminousColor");

	registerClass<::PointLightComponent>("PointLightComponent")
		.registerProperty(&::LightComponent::luminousColor, "luminousColor")
		.registerProperty(&::PointLightComponent::radius, "radius");

	registerClass<::DirectionalLightComponent>("DirectionalLightComponent")
		.registerProperty(&::LightComponent::luminousColor, "luminousColor")
		.registerProperty(&::DirectionalLightComponent::direction, "direction")
		.registerProperty(&::DirectionalLightComponent::aspectRatio, "aspectRatio");
#endif

}

}

}

#endif
