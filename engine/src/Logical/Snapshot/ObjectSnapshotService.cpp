#include "Logical/Snapshot/ObjectSnapshotService.hpp"

#include "AssetManager/ModelImporter.hpp"
#include "Logical/Animation/Animation.hpp"
#include "Logical/Framework/Component/AnimationComponent.hpp"
#include "Logical/Framework/Component/CameraComponent.hpp"
#include "Logical/Framework/Component/LightComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"
#include "Logical/Framework/Component/TransformComponent.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "Logical/Framework/World/Scene.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace Snapshot {
namespace {

bool equalVec3(const Vec3& lhs, const Vec3& rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool equalTransformDTO(const TransformDTO& lhs, const TransformDTO& rhs)
{
	return equalVec3(lhs.translation, rhs.translation)
		&& equalVec3(lhs.rotation, rhs.rotation)
		&& equalVec3(lhs.scale, rhs.scale);
}

bool equalMaterialTexturesDTO(const MaterialTexturesDTO& lhs, const MaterialTexturesDTO& rhs)
{
	return lhs.diffuse == rhs.diffuse
		&& lhs.specular == rhs.specular
		&& lhs.normal == rhs.normal
		&& lhs.height == rhs.height
		&& lhs.albedo == rhs.albedo
		&& lhs.metallic == rhs.metallic
		&& lhs.roughness == rhs.roughness
		&& lhs.ao == rhs.ao;
}

bool equalMaterialDTO(const MaterialDTO& lhs, const MaterialDTO& rhs)
{
	return lhs.alpha == rhs.alpha
		&& equalVec3(lhs.base_color_factor, rhs.base_color_factor)
		&& lhs.metallic_factor == rhs.metallic_factor
		&& lhs.roughness_factor == rhs.roughness_factor
		&& lhs.ao_factor == rhs.ao_factor
		&& equalVec3(lhs.diffuse_factor, rhs.diffuse_factor)
		&& equalVec3(lhs.specular_factor, rhs.specular_factor)
		&& lhs.shininess == rhs.shininess
		&& equalMaterialTexturesDTO(lhs.textures, rhs.textures);
}

bool equalSubMeshDTO(const SubMeshDTO& lhs, const SubMeshDTO& rhs)
{
	return lhs.sub_mesh_index == rhs.sub_mesh_index
		&& equalTransformDTO(lhs.local_transform, rhs.local_transform);
}

bool equalPointLightDTO(const PointLightDTO& lhs, const PointLightDTO& rhs)
{
	return equalVec3(lhs.luminous_color, rhs.luminous_color)
		&& lhs.radius == rhs.radius
		&& lhs.cast_shadow == rhs.cast_shadow;
}

bool equalDirectionalLightDTO(const DirectionalLightDTO& lhs, const DirectionalLightDTO& rhs)
{
	return equalVec3(lhs.luminous_color, rhs.luminous_color)
		&& equalVec3(lhs.direction, rhs.direction)
		&& lhs.aspect_ratio == rhs.aspect_ratio;
}

bool equalCameraDTO(const CameraDTO& lhs, const CameraDTO& rhs)
{
	return lhs.mode == rhs.mode
		&& lhs.projection_mode == rhs.projection_mode
		&& lhs.zoom_mode == rhs.zoom_mode
		&& lhs.origin_fov == rhs.origin_fov
		&& lhs.fov == rhs.fov
		&& lhs.near_plane == rhs.near_plane
		&& lhs.far_plane == rhs.far_plane
		&& lhs.aspect_ratio == rhs.aspect_ratio
		&& equalVec3(lhs.position, rhs.position)
		&& equalVec3(lhs.direction, rhs.direction)
		&& equalVec3(lhs.up_direction, rhs.up_direction);
}

bool equalObjectDTO(const ObjectDTO& lhs, const ObjectDTO& rhs)
{
	if (lhs.name != rhs.name
		|| lhs.visible != rhs.visible
		|| !equalTransformDTO(lhs.transform, rhs.transform)
		|| lhs.filepath != rhs.filepath
		|| lhs.file_type != rhs.file_type
		|| lhs.static_shadow_caster != rhs.static_shadow_caster
		|| lhs.has_point_light != rhs.has_point_light
		|| lhs.has_directional_light != rhs.has_directional_light
		|| lhs.has_camera != rhs.has_camera
		|| lhs.sub_meshes.size() != rhs.sub_meshes.size()
		|| lhs.materials.size() != rhs.materials.size()) {
		return false;
	}

	for (size_t i = 0; i < lhs.sub_meshes.size(); ++i) {
		if (!equalSubMeshDTO(lhs.sub_meshes[i], rhs.sub_meshes[i]))
			return false;
	}
	for (size_t i = 0; i < lhs.materials.size(); ++i) {
		if (!equalMaterialDTO(lhs.materials[i], rhs.materials[i]))
			return false;
	}

	return equalPointLightDTO(lhs.point_light, rhs.point_light)
		&& equalDirectionalLightDTO(lhs.directional_light, rhs.directional_light)
		&& equalCameraDTO(lhs.camera, rhs.camera);
}

std::shared_ptr<Texture> makeTexture(const std::string& filepath, TextureType type, bool gamma)
{
	return filepath.empty() ? nullptr : std::make_shared<Texture>(type, filepath, gamma);
}

void applyMaterialDTO(Material& material, const MaterialDTO& dto)
{
	material.alpha = dto.alpha;
	material.base_color_factor = dto.base_color_factor;
	material.metallic_factor = dto.metallic_factor;
	material.roughness_factor = dto.roughness_factor;
	material.ao_factor = dto.ao_factor;
	material.diffuse_factor = dto.diffuse_factor;
	material.specular_factor = dto.specular_factor;
	material.shininess = dto.shininess;
	material.diffuse_texture = makeTexture(dto.textures.diffuse, TextureType::Diffuse, false);
	material.specular_texture = makeTexture(dto.textures.specular, TextureType::Specular, false);
	material.normal_texture = makeTexture(dto.textures.normal, TextureType::Normal, false);
	material.height_texture = makeTexture(dto.textures.height, TextureType::Height, false);
	material.albedo_texture = makeTexture(dto.textures.albedo, TextureType::Albedo, true);
	material.metallic_texture = makeTexture(dto.textures.metallic, TextureType::Metallic, false);
	material.roughness_texture = makeTexture(dto.textures.roughness, TextureType::Roughness, false);
	material.ao_texture = makeTexture(dto.textures.ao, TextureType::AO, false);
	material.markDirty();
}

bool shouldCreateTransform(const ObjectDTO& dto)
{
	if (!dto.filepath.empty() || dto.has_camera || dto.has_point_light)
		return true;
	if (!dto.has_directional_light)
		return true;
	return false;
}

} // namespace

ObjectSnapshot ObjectSnapshotService::capture(Scene& scene, GObjectID id)
{
	ObjectSnapshot snapshot;
	snapshot.id = id;

	GObject* object = scene.objectOf(id);
	if (!object)
		return snapshot;

	snapshot.existed = true;
	snapshot.dto = buildDTOFromObject(*object);
	return snapshot;
}

void ObjectSnapshotService::restore(Scene& scene, const ObjectSnapshot& snapshot)
{
	GObject* object = scene.objectOf(snapshot.id);
	if (!snapshot.existed) {
		if (object)
			scene.removeObject(snapshot.id);
		return;
	}

	if (!object)
		object = createObjectFromDTO(scene, snapshot.id, snapshot.dto);
	if (!object)
		return;

	applyDTOToObject(scene, *object, snapshot.dto);
	object->markDirty(dirtyFlagsForDTO(snapshot.dto));
}

bool ObjectSnapshotService::equals(const ObjectSnapshot& lhs, const ObjectSnapshot& rhs)
{
	return lhs.id == rhs.id
		&& lhs.existed == rhs.existed
		&& (!lhs.existed || equalObjectDTO(lhs.dto, rhs.dto));
}

ObjectDTO ObjectSnapshotService::buildDTOFromObject(GObject& object)
{
	ObjectDTO dto;
	dto.name = object.name();
	dto.visible = object.visible();

	const TransformComponent* transform = object.getComponent<TransformComponent>();
	const MeshComponent* mesh = object.getComponent<MeshComponent>();
	const CameraComponent* camera = object.getComponent<CameraComponent>();
	const PointLightComponent* point_light = object.getComponent<PointLightComponent>();
	const DirectionalLightComponent* directional_light = object.getComponent<DirectionalLightComponent>();

	dto.transform.translation = transform ? transform->translation : Vec3(0.0f);
	dto.transform.rotation = transform ? transform->rotation : Vec3(0.0f);
	dto.transform.scale = transform ? transform->scale : Vec3(1.0f);

	if (mesh && !mesh->source_filepath.empty()) {
		dto.filepath = mesh->source_filepath;
		dto.file_type = static_cast<int>(FileType::OBJ);
	}

	if (point_light) {
		dto.has_point_light = true;
		dto.point_light.luminous_color = point_light->luminousColor;
		dto.point_light.radius = point_light->radius;
		dto.point_light.cast_shadow = point_light->castShadow;
	}
	if (directional_light) {
		dto.has_directional_light = true;
		dto.directional_light.luminous_color = directional_light->luminousColor;
		dto.directional_light.direction = directional_light->direction;
		dto.directional_light.aspect_ratio = directional_light->aspectRatio;
	}
	if (camera) {
		dto.has_camera = true;
		dto.camera.mode = static_cast<int>(camera->mode);
		dto.camera.projection_mode = static_cast<int>(camera->projection_mode);
		dto.camera.zoom_mode = static_cast<int>(camera->zoom_mode);
		dto.camera.origin_fov = camera->originFov;
		dto.camera.fov = camera->fov;
		dto.camera.near_plane = camera->nearPlane;
		dto.camera.far_plane = camera->farPlane;
		dto.camera.aspect_ratio = camera->aspectRatio;
		dto.camera.position = camera->pos;
		dto.camera.direction = camera->direction;
		dto.camera.up_direction = camera->upDirection;
	}

	if (mesh) {
		dto.static_shadow_caster = mesh->staticShadowCaster;
		for (const auto& sub_mesh : mesh->sub_meshes) {
			if (!sub_mesh)
				continue;

			SubMeshDTO sub_mesh_dto;
			sub_mesh_dto.sub_mesh_index = sub_mesh->sub_mesh_idx;
			sub_mesh_dto.local_transform.translation = sub_mesh->translation;
			sub_mesh_dto.local_transform.rotation = sub_mesh->rotation;
			sub_mesh_dto.local_transform.scale = sub_mesh->scale;
			dto.sub_meshes.push_back(std::move(sub_mesh_dto));

			MaterialDTO material_dto;
			if (sub_mesh->material) {
				material_dto.alpha = sub_mesh->material->alpha;
				material_dto.base_color_factor = sub_mesh->material->base_color_factor;
				material_dto.metallic_factor = sub_mesh->material->metallic_factor;
				material_dto.roughness_factor = sub_mesh->material->roughness_factor;
				material_dto.ao_factor = sub_mesh->material->ao_factor;
				material_dto.diffuse_factor = sub_mesh->material->diffuse_factor;
				material_dto.specular_factor = sub_mesh->material->specular_factor;
				material_dto.shininess = sub_mesh->material->shininess;

				auto pathOf = [](const std::shared_ptr<Texture>& texture) {
					return texture ? texture->texture_filepath : std::string();
				};
				material_dto.textures.diffuse = pathOf(sub_mesh->material->diffuse_texture);
				material_dto.textures.specular = pathOf(sub_mesh->material->specular_texture);
				material_dto.textures.normal = pathOf(sub_mesh->material->normal_texture);
				material_dto.textures.height = pathOf(sub_mesh->material->height_texture);
				material_dto.textures.albedo = pathOf(sub_mesh->material->albedo_texture);
				material_dto.textures.metallic = pathOf(sub_mesh->material->metallic_texture);
				material_dto.textures.roughness = pathOf(sub_mesh->material->roughness_texture);
				material_dto.textures.ao = pathOf(sub_mesh->material->ao_texture);
			}
			dto.materials.push_back(std::move(material_dto));
		}
	}

	return dto;
}

void ObjectSnapshotService::applyDTOToObject(Scene& scene, GObject& object, const ObjectDTO& dto)
{
	applyDTOToObject(scene.registry(), object, dto);
}

void ObjectSnapshotService::applyDTOToObject(SceneObjectRegistry& registry, GObject& object, const ObjectDTO& dto)
{
	object.setName(dto.name.empty() ? object.name() : dto.name);
	object.setVisible(dto.visible);

	if (auto* transform = object.getComponent<TransformComponent>()) {
		transform->translation = dto.transform.translation;
		transform->rotation = dto.transform.rotation;
		transform->scale = dto.transform.scale;
	}

	if (auto* mesh = object.getComponent<MeshComponent>()) {
		mesh->source_filepath = dto.filepath;
		mesh->staticShadowCaster = dto.static_shadow_caster;
		if (!dto.sub_meshes.empty()) {
			std::vector<std::shared_ptr<Mesh>> filtered;
			filtered.reserve(dto.sub_meshes.size());
			for (const SubMeshDTO& sub_mesh_dto : dto.sub_meshes) {
				auto it = std::find_if(mesh->sub_meshes.begin(), mesh->sub_meshes.end(),
					[&sub_mesh_dto](const std::shared_ptr<Mesh>& sub_mesh) {
						return sub_mesh && sub_mesh->sub_mesh_idx == sub_mesh_dto.sub_mesh_index;
					});
				if (it != mesh->sub_meshes.end())
					filtered.push_back(*it);
			}
			if (!filtered.empty())
				mesh->sub_meshes = std::move(filtered);
		}

		const size_t n = std::min({ mesh->sub_meshes.size(), dto.sub_meshes.size(), dto.materials.size() });
		for (size_t i = 0; i < n; ++i) {
			auto& sub_mesh = mesh->sub_meshes[i];
			if (!sub_mesh)
				continue;

			const TransformDTO& local_transform = dto.sub_meshes[i].local_transform;
			sub_mesh->translation = local_transform.translation;
			sub_mesh->rotation = local_transform.rotation;
			sub_mesh->scale = local_transform.scale;

			if (!sub_mesh->material)
				sub_mesh->material = std::make_shared<Material>();
			applyMaterialDTO(*sub_mesh->material, dto.materials[i]);
		}
	}

	if (dto.has_point_light) {
		auto* point_light = object.getComponent<PointLightComponent>();
		if (!point_light)
			point_light = &object.addComponent<PointLightComponent>();
		point_light->luminousColor = dto.point_light.luminous_color;
		point_light->radius = dto.point_light.radius;
		point_light->castShadow = dto.point_light.cast_shadow;
	}

	if (dto.has_directional_light) {
		auto* directional_light = object.getComponent<DirectionalLightComponent>();
		if (!directional_light)
			directional_light = &object.addComponent<DirectionalLightComponent>();
		directional_light->luminousColor = dto.directional_light.luminous_color;
		directional_light->direction = dto.directional_light.direction;
		directional_light->aspectRatio = dto.directional_light.aspect_ratio;
	}

	if (dto.has_camera) {
		auto* camera = object.getComponent<CameraComponent>();
		if (!camera)
			camera = &object.addComponent<CameraComponent>();
		camera->mode = static_cast<Mode>(dto.camera.mode);
		camera->projection_mode = static_cast<Projection>(dto.camera.projection_mode);
		camera->zoom_mode = static_cast<ZoomMode>(dto.camera.zoom_mode);
		camera->originFov = dto.camera.origin_fov;
		camera->fov = dto.camera.fov;
		camera->nearPlane = dto.camera.near_plane;
		camera->farPlane = dto.camera.far_plane;
		camera->aspectRatio = dto.camera.aspect_ratio;
		camera->pos = dto.camera.position;
		camera->direction = dto.camera.direction;
		camera->upDirection = dto.camera.up_direction;
		camera->refreshView();
		camera->refreshProjection();
		registry.setMainCameraObjectId(object.ID());

		if (auto* transform = object.getComponent<TransformComponent>())
			transform->translation = camera->pos;
	}
}

SceneDirtyFlags ObjectSnapshotService::dirtyFlagsForDTO(const ObjectDTO& dto)
{
	SceneDirtyFlags flags = SceneDirtyFlagBit(SceneDirtyFlag::Visibility)
		| SceneDirtyFlagBit(SceneDirtyFlag::Transform);
	if (!dto.filepath.empty() || !dto.sub_meshes.empty())
		flags |= SceneDirtyFlagBit(SceneDirtyFlag::Mesh);
	if (!dto.materials.empty())
		flags |= SceneDirtyFlagBit(SceneDirtyFlag::Material);
	if (dto.has_point_light || dto.has_directional_light)
		flags |= SceneDirtyFlagBit(SceneDirtyFlag::Light);
	if (dto.has_camera)
		flags |= SceneDirtyFlagBit(SceneDirtyFlag::Camera);
	return flags;
}

GObject* ObjectSnapshotService::createObjectFromDTO(Scene& scene, GObjectID id, const ObjectDTO& dto)
{
	const std::string name = dto.name.empty() ? "Object" : dto.name;
	GObject* object = GObject::createWithID(nullptr, name, id);

	if (shouldCreateTransform(dto))
		object->addComponent<TransformComponent>();

	if (!dto.filepath.empty()) {
		ModelImporter model_importer;
		if (model_importer.load(dto.filepath)) {
			MeshComponent& mesh = object->addComponent<MeshComponent>();
			mesh.source_filepath = dto.filepath;
			mesh.staticShadowCaster = dto.static_shadow_caster;
			mesh.sub_meshes = model_importer.meshes();
			if (model_importer.hasAnimation()) {
				AnimationComponent& animation = object->addComponent<AnimationComponent>();
				animation.clip_path = dto.filepath;
				animation.clip = std::make_shared<Animation>(dto.filepath, &model_importer);
			}
		}
	}

	if (dto.has_point_light)
		object->addComponent<PointLightComponent>();
	if (dto.has_directional_light)
		object->addComponent<DirectionalLightComponent>();
	if (dto.has_camera)
		object->addComponent<CameraComponent>();

	scene.addObject(std::shared_ptr<GObject>(object));
	return object;
}

} // namespace Snapshot
