#include "Logical/Framework/World/ProjectSerializer.hpp"
#include "Logical/Framework/World/SceneObjectRegistry.hpp"
#include "Logical/Framework/World/SceneDirtyTracker.hpp"
#include "Logical/Framework/World/SelectionManager.hpp"

#include "Logical/Framework/Component/CameraComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"
#include "Logical/Framework/Component/LightComponent.hpp"
#include "Logical/Framework/Component/TransformComponent.hpp"

#include "AssetManager/DTO.hpp"

ProjectSerializer::ProjectSerializer(SceneObjectRegistry& registry,
	SceneDirtyTracker& dirty_tracker, SelectionManager& selection)
	: m_registry(registry)
	, m_dirty_tracker(dirty_tracker)
	, m_selection(selection)
{
}

bool ProjectSerializer::loadProject(const std::string& project_filepath, bool clear_old)
{
	m_current_project_filepath = project_filepath;
	ProjectDTO dto;
	Meta::Serialization::Serializer::loadFromJsonFile(project_filepath, dto);
	applyProjectDTOToScene(dto, clear_old);
	return true;
}

bool ProjectSerializer::saveProject(const std::string& project_filepath)
{
	m_current_project_filepath = project_filepath;
	ProjectDTO dto = buildProjectDTOFromScene(m_current_project_filepath);
	Meta::Serialization::Serializer::saveToJsonFile(project_filepath, dto);
	return true;
}

ProjectDTO ProjectSerializer::buildProjectDTOFromScene(const std::string& project_filepath)
{
	ProjectDTO dto;
	dto.project_name = PathService::getFileName(project_filepath);
	const std::string project_dir = PathService::getDirectory(project_filepath);

	for (const auto& obj_sp : m_registry.getObjects()) {
		if (!obj_sp) continue;
		GObject& obj = *obj_sp;
		const TransformComponent* tc = obj.getComponent<TransformComponent>();
		const MeshComponent* mc = obj.getComponent<MeshComponent>();
		const CameraComponent* camera = obj.getComponent<CameraComponent>();
		const PointLightComponent* point_light = obj.getComponent<PointLightComponent>();
		const DirectionalLightComponent* directional_light = obj.getComponent<DirectionalLightComponent>();
		if (!mc && !camera && !point_light && !directional_light) continue;

		ObjectDTO obj_dto;
		obj_dto.name = obj.name();
		obj_dto.visible = obj.visible();
		obj_dto.transform.translation = tc ? tc->translation : Vec3(0.0f);
		obj_dto.transform.rotation = tc ? tc->rotation : Vec3(0.0f);
		obj_dto.transform.scale = tc ? tc->scale : Vec3(1.0f);
		if (mc && !mc->source_filepath.empty()) {
			obj_dto.filepath = PathService::tryMakeRelative(project_dir, mc->source_filepath);
			obj_dto.file_type = static_cast<int>(FileType::OBJ);
		}

		if (point_light) {
			obj_dto.has_point_light = true;
			obj_dto.point_light.luminous_color = point_light->luminousColor;
			obj_dto.point_light.radius = point_light->radius;
		}
		if (directional_light) {
			obj_dto.has_directional_light = true;
			obj_dto.directional_light.luminous_color = directional_light->luminousColor;
			obj_dto.directional_light.direction = directional_light->direction;
			obj_dto.directional_light.aspect_ratio = directional_light->aspectRatio;
		}
		if (camera) {
			obj_dto.has_camera = true;
			obj_dto.camera.mode = static_cast<int>(camera->mode);
			obj_dto.camera.projection_mode = static_cast<int>(camera->projection_mode);
			obj_dto.camera.zoom_mode = static_cast<int>(camera->zoom_mode);
			obj_dto.camera.origin_fov = camera->originFov;
			obj_dto.camera.fov = camera->fov;
			obj_dto.camera.near_plane = camera->nearPlane;
			obj_dto.camera.far_plane = camera->farPlane;
			obj_dto.camera.aspect_ratio = camera->aspectRatio;
			obj_dto.camera.position = camera->pos;
			obj_dto.camera.direction = camera->direction;
			obj_dto.camera.up_direction = camera->upDirection;
		}

		if (mc) {
			for (const auto& sub : mc->sub_meshes) {
				if (!sub) continue;

				SubMeshDTO sm;
				sm.sub_mesh_index = sub->sub_mesh_idx;
				sm.local_transform.translation = sub->translation;
				sm.local_transform.rotation = sub->rotation;
				sm.local_transform.scale = sub->scale;
				obj_dto.sub_meshes.push_back(std::move(sm));

				MaterialDTO mat_dto;
				if (sub->material) {
					mat_dto.alpha = sub->material->alpha;
					mat_dto.base_color_factor = sub->material->base_color_factor;
					mat_dto.metallic_factor = sub->material->metallic_factor;
					mat_dto.roughness_factor = sub->material->roughness_factor;
					mat_dto.ao_factor = sub->material->ao_factor;
					mat_dto.diffuse_factor = sub->material->diffuse_factor;
					mat_dto.specular_factor = sub->material->specular_factor;
					mat_dto.shininess = sub->material->shininess;
					auto rel = [&project_dir](const std::shared_ptr<Texture>& t) {
						return t ? PathService::tryMakeRelative(project_dir, t->texture_filepath) : std::string();
					};
					mat_dto.textures.diffuse = rel(sub->material->diffuse_texture);
					mat_dto.textures.specular = rel(sub->material->specular_texture);
					mat_dto.textures.normal = rel(sub->material->normal_texture);
					mat_dto.textures.height = rel(sub->material->height_texture);
					mat_dto.textures.albedo = rel(sub->material->albedo_texture);
					mat_dto.textures.metallic = rel(sub->material->metallic_texture);
					mat_dto.textures.roughness = rel(sub->material->roughness_texture);
					mat_dto.textures.ao = rel(sub->material->ao_texture);
				}
				obj_dto.materials.push_back(std::move(mat_dto));
			}
		}
		dto.objects.push_back(std::move(obj_dto));
	}
	return dto;
}

void ProjectSerializer::applyProjectDTOToScene(const ProjectDTO& dto, bool clear_old)
{
	if (clear_old) {
		std::shared_ptr<GObject> main_camera_object;
		int main_camera_id = m_registry.mainCameraObjectId();
		for (const auto& obj : m_registry.getObjects()) {
			if (obj && obj->ID().id == main_camera_id && obj->hasComponent<CameraComponent>()) {
				main_camera_object = obj;
				break;
			}
		}
		m_registry.clear();
		if (main_camera_object)
			m_registry.registerObject(main_camera_object, false);
		else
			m_registry.createCamera();
		m_selection.clear();
		m_dirty_tracker.markFullResync();
	}

	const std::string project_dir = PathService::getDirectory(m_current_project_filepath);

	for (const auto& obj_dto : dto.objects) {
		const FileType ft = static_cast<FileType>(obj_dto.file_type);
		GObject* obj = nullptr;
		std::string model_abs;
		if (!obj_dto.filepath.empty()) {
			model_abs = PathService::join(project_dir, obj_dto.filepath);
		}
		if (obj_dto.has_camera && model_abs.empty())
			obj = m_registry.mainCameraObject() ? m_registry.mainCameraObject()
				: m_registry.createCamera(obj_dto.name.empty() ? "Main Camera" : obj_dto.name);
		if (!obj && !model_abs.empty() && (ft == FileType::OBJ || ft == FileType::None))
			obj = m_registry.loadModel(model_abs);
		if (!obj && (obj_dto.has_point_light || obj_dto.has_directional_light))
			obj = m_registry.createObject(obj_dto.name.empty() ? "Light" : obj_dto.name, obj_dto.has_point_light);
		if (!obj) continue;

		obj->setName(obj_dto.name.empty() ? obj->name() : obj_dto.name);
		obj->setVisible(obj_dto.visible);
		if (auto* tc = obj->getComponent<TransformComponent>()) {
			tc->translation = obj_dto.transform.translation;
			tc->rotation = obj_dto.transform.rotation;
			tc->scale = obj_dto.transform.scale;
		}
		if (auto* mc = obj->getComponent<MeshComponent>()) {
			mc->source_filepath = model_abs;
			if (!obj_dto.sub_meshes.empty()) {
				std::vector<std::shared_ptr<Mesh>> filtered;
				filtered.reserve(obj_dto.sub_meshes.size());
				for (const auto& sm : obj_dto.sub_meshes) {
					const int expected_idx = sm.sub_mesh_index;
					auto it = std::find_if(mc->sub_meshes.begin(), mc->sub_meshes.end(),
						[expected_idx](const std::shared_ptr<Mesh>& m) { return m && m->sub_mesh_idx == expected_idx; });
					if (it != mc->sub_meshes.end()) filtered.push_back(*it);
				}
				if (!filtered.empty()) mc->sub_meshes = std::move(filtered);
			}
			const size_t n = std::min({ mc->sub_meshes.size(), obj_dto.sub_meshes.size(), obj_dto.materials.size() });
			for (size_t i = 0; i < n; i++) {
				auto& sub = mc->sub_meshes[i];
				if (!sub) continue;
				const TransformDTO& lt = obj_dto.sub_meshes[i].local_transform;
				sub->translation = lt.translation;
				sub->rotation = lt.rotation;
				sub->scale = lt.scale;

				if (!sub->material) sub->material = std::make_shared<Material>();
				const auto& md = obj_dto.materials[i];
				sub->material->alpha = md.alpha;
				sub->material->base_color_factor = md.base_color_factor;
				sub->material->metallic_factor = md.metallic_factor;
				sub->material->roughness_factor = md.roughness_factor;
				sub->material->ao_factor = md.ao_factor;
				sub->material->diffuse_factor = md.diffuse_factor;
				sub->material->specular_factor = md.specular_factor;
				sub->material->shininess = md.shininess;
				auto mktex = [&project_dir](const std::string& rel, TextureType type, bool gamma) -> std::shared_ptr<Texture> {
					return rel.empty() ? nullptr : std::make_shared<Texture>(type, PathService::join(project_dir, rel), gamma);
				};
				sub->material->diffuse_texture = mktex(md.textures.diffuse, TextureType::Diffuse, false);
				sub->material->specular_texture = mktex(md.textures.specular, TextureType::Specular, false);
				sub->material->normal_texture = mktex(md.textures.normal, TextureType::Normal, false);
				sub->material->height_texture = mktex(md.textures.height, TextureType::Height, false);
				sub->material->albedo_texture = mktex(md.textures.albedo, TextureType::Albedo, true);
				sub->material->metallic_texture = mktex(md.textures.metallic, TextureType::Metallic, false);
				sub->material->roughness_texture = mktex(md.textures.roughness, TextureType::Roughness, false);
				sub->material->ao_texture = mktex(md.textures.ao, TextureType::AO, false);
				sub->material->markDirty();
			}
		}
		if (obj_dto.has_point_light) {
			auto* point_light = obj->getComponent<PointLightComponent>();
			if (!point_light)
				point_light = &obj->addComponent<PointLightComponent>();
			point_light->luminousColor = obj_dto.point_light.luminous_color;
			point_light->radius = obj_dto.point_light.radius;
		}
		if (obj_dto.has_directional_light) {
			auto* directional_light = obj->getComponent<DirectionalLightComponent>();
			if (!directional_light)
				directional_light = &obj->addComponent<DirectionalLightComponent>();
			directional_light->luminousColor = obj_dto.directional_light.luminous_color;
			directional_light->direction = obj_dto.directional_light.direction;
			directional_light->aspectRatio = obj_dto.directional_light.aspect_ratio;
		}
		if (obj_dto.has_camera) {
			auto* camera = obj->getComponent<CameraComponent>();
			if (!camera)
				camera = &obj->addComponent<CameraComponent>();
			camera->mode = static_cast<Mode>(obj_dto.camera.mode);
			camera->projection_mode = static_cast<Projection>(obj_dto.camera.projection_mode);
			camera->zoom_mode = static_cast<ZoomMode>(obj_dto.camera.zoom_mode);
			camera->originFov = obj_dto.camera.origin_fov;
			camera->fov = obj_dto.camera.fov;
			camera->nearPlane = obj_dto.camera.near_plane;
			camera->farPlane = obj_dto.camera.far_plane;
			camera->aspectRatio = obj_dto.camera.aspect_ratio;
			camera->pos = obj_dto.camera.position;
			camera->direction = obj_dto.camera.direction;
			camera->upDirection = obj_dto.camera.up_direction;
			camera->refreshView();
			camera->refreshProjection();
			m_registry.setMainCameraObjectId(obj->ID().id);

			if (auto* transform = obj->getComponent<TransformComponent>())
				transform->translation = camera->pos;
		}

		SceneDirtyFlags object_dirty =
			SceneDirtyFlagBit(SceneDirtyFlag::Visibility) |
			SceneDirtyFlagBit(SceneDirtyFlag::Transform);
		if (obj->hasComponent<MeshComponent>())
			object_dirty |= SceneDirtyFlagBit(SceneDirtyFlag::Mesh) | SceneDirtyFlagBit(SceneDirtyFlag::Material);
		if (obj->hasComponent<PointLightComponent>() || obj->hasComponent<DirectionalLightComponent>())
			object_dirty |= SceneDirtyFlagBit(SceneDirtyFlag::Light);
		if (obj->hasComponent<CameraComponent>())
			object_dirty |= SceneDirtyFlagBit(SceneDirtyFlag::Camera);
		obj->markDirty(object_dirty);
	}

	if (!m_registry.mainCameraObject())
		m_registry.createCamera();
	if (!m_registry.mainDirectionalLightObject())
		m_registry.createDirectionalLight();
}
