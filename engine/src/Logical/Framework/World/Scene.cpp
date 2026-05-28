#include "Logical/Framework/World/Scene.hpp"

#include "Logical/Framework/Component/CameraComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"
#include "Logical/Framework/Component/LightComponent.hpp"
#include "Logical/Framework/Component/TransformComponent.hpp"
#include "Logical/Framework/Component/AnimationComponent.hpp"
#include "Logical/Animation/Animation.hpp"

#include "AssetManager/DTO.hpp"
#include "AssetManager/ModelImporter.hpp"

#include <cassert>

namespace
{
void addUniqueID(std::vector<GObjectID>& ids, GObjectID id)
{
	if (std::find(ids.begin(), ids.end(), id) == ids.end())
		ids.push_back(id);
}

void removeID(std::vector<GObjectID>& ids, GObjectID id)
{
	ids.erase(std::remove(ids.begin(), ids.end(), id), ids.end());
}
}

Scene::Scene()
{
	createCamera();
	createDirectionalLight();
}

GObject* Scene::createObject(const std::string& name, bool with_transform)
{
	auto obj = GObject::create(nullptr, name);
	if (with_transform)
		obj->addComponent<TransformComponent>();
	registerObject(std::shared_ptr<GObject>(obj));
	return obj;
}

GObject* Scene::createCamera(const std::string& name)
{
	GObject* obj = createObject(name);
	auto& camera = obj->addComponent<CameraComponent>();
	camera.refreshView();
	camera.refreshProjection();

	// CameraComponent 仍然维护渲染用的 pos/direction/view；
	// TransformComponent 则让相机作为普通 GObject 参与层级、选择和序列化。
	// 创建时先把 Transform 的位置对齐到相机当前位置，后续由 CameraManipulator
	// 和 Gizmo 负责继续同步。
	if (auto* transform = obj->getComponent<TransformComponent>())
		transform->translation = camera.pos;

	m_main_camera_object_id = obj->ID().id;
	obj->markRenderDirty(
		RenderDirtyFlagBit(RenderDirtyFlag::Transform) |
		RenderDirtyFlagBit(RenderDirtyFlag::Camera));
	return obj;
}

GObject* Scene::createDirectionalLight(const std::string& name)
{
	GObject* obj = createObject(name, false);
	auto& light = obj->addComponent<DirectionalLightComponent>();
	light.luminousColor = Color3(1.0f);
	light.direction = { 15.0f, -30.0f, 15.0f };
	obj->markRenderDirty(RenderDirtyFlagBit(RenderDirtyFlag::Light));
	return obj;
}

GObject* Scene::createPointLight(const std::string& name)
{
	GObject* obj = createObject(name);
	auto& transform = *obj->getComponent<TransformComponent>();
	transform.translation = {
		static_cast<float>(Math::random(-15.0f, 15.0f)),
		static_cast<float>(Math::random(1.0f, 30.0f)),
		static_cast<float>(Math::random(-15.0f, 15.0f))
	};
	obj->markRenderDirty(RenderDirtyFlagBit(RenderDirtyFlag::Transform));

	auto& light = obj->addComponent<PointLightComponent>();
	light.radius = 30.0f;
	light.luminousColor = Color3(
		static_cast<float>(Math::randomUnit()),
		static_cast<float>(Math::randomUnit()),
		static_cast<float>(Math::randomUnit()));
	obj->markRenderDirty(RenderDirtyFlagBit(RenderDirtyFlag::Light));
	return obj;
}

void Scene::removeLastPointLight()
{
	for (auto it = m_objects.end(); it != m_objects.begin();)
	{
		--it;
		if (!(*it) || !(*it)->hasComponent<PointLightComponent>())
			continue;

		removeObject((*it)->ID());
		return;
	}
}

GObject* Scene::loadModel(const std::string& filepath)
{
	ModelImporter model_importer;
	if (!model_importer.load(filepath))
		return nullptr;
	std::vector<int> obj_sub_meshes_idx = model_importer.getSubMeshesIds();
	if (obj_sub_meshes_idx.empty()) {
		//Logger::error("Model datas is empty. File loading fails. Please check if the filepath is all English.");
		return nullptr;
	}
	std::string name = PathService::getFileName(filepath);

#if ENABLE_ECS
	auto& world = ecs::World::get();
	auto entity = world.create_entity();
	world.addComponent<ecs::NameComponent>(entity).name = name;
	world.addComponent<TransformComponent>(entity);
	world.addComponent<ExplosionComponent>(entity);
	auto& renderable = world.addComponent<ecs::RenderableComponent>(entity);
	for (int idx : obj_sub_meshes_idx) {
		renderable.sub_meshes.push_back(Mesh{ idx, MeshFileRef{ MeshFileType::OBJ, filepath}, {}, Mat4(1.0f) });
	}
	auto res = GObject::create(nullptr, entity);
#else
	auto res = GObject::create(nullptr, name);
	res->addComponent<TransformComponent>();
	MeshComponent& mesh = res->addComponent<MeshComponent>();
	mesh.source_filepath = filepath;
	for (int idx : obj_sub_meshes_idx) {
		std::shared_ptr<Mesh> sub_mesh = model_importer.meshOfNode(idx);
		sub_mesh->sub_mesh_idx = idx;
		mesh.sub_meshes.push_back(sub_mesh);
	}
	if (model_importer.hasAnimation()) {
		AnimationComponent& animation = res->addComponent<AnimationComponent>();
		animation.clip_path = filepath;
		animation.clip = std::make_shared<Animation>(filepath, &model_importer);
	}
	registerObject(std::shared_ptr<GObject>(res));
#endif

	return res;
}

// Scene -> DTO
ProjectDTO Scene::buildProjectDTOFromScene(const std::string& project_filepath)
{
	ProjectDTO dto;
    dto.project_name = PathService::getFileName(project_filepath);
	const std::string project_dir = PathService::getDirectory(project_filepath);

	for (const auto& obj_sp : this->getObjects()) {
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

// DTO -> Scene
void Scene::applyProjectDTOToScene(const ProjectDTO& dto, bool clear_old)
{
    if (clear_old) {
		// CameraManipulator 持有 Main Camera 的 CameraComponent 引用。
		// 因此清空场景时保留主相机对象本身，只重载它的数据；这样 GUI 输入系统
		// 不会因为项目切换而拿到悬空引用。项目文件里的 CameraDTO 会覆盖其状态。
		std::shared_ptr<GObject> main_camera_object;
		for (const auto& obj : m_objects) {
			if (obj && obj->ID().id == m_main_camera_object_id && obj->hasComponent<CameraComponent>()) {
				main_camera_object = obj;
				break;
			}
		}
        this->m_objects.clear();
		this->m_object_by_id.clear();
		this->m_directional_light_object_ids.clear();
		this->m_point_light_object_ids.clear();
		if (main_camera_object)
			registerObject(main_camera_object, false);
		else
			createCamera();
        this->m_picked_objects.clear();
		markFullRenderResync();
    }

	const std::string project_dir = PathService::getDirectory(this->m_current_project_filepath);

	for (const auto& obj_dto : dto.objects) {
		const FileType ft = static_cast<FileType>(obj_dto.file_type);
		GObject* obj = nullptr;
		std::string model_abs;
		if (!obj_dto.filepath.empty()) {
			model_abs = PathService::join(project_dir, obj_dto.filepath);
		}
		if (obj_dto.has_camera && model_abs.empty())
			obj = mainCameraObject() ? mainCameraObject() : createCamera(obj_dto.name.empty() ? "Main Camera" : obj_dto.name);
		if (!obj && !model_abs.empty() && (ft == FileType::OBJ || ft == FileType::None))
			obj = this->loadModel(model_abs);
		if (!obj && (obj_dto.has_point_light || obj_dto.has_directional_light))
			obj = this->createObject(obj_dto.name.empty() ? "Light" : obj_dto.name, obj_dto.has_point_light);
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
			m_main_camera_object_id = obj->ID().id;

			if (auto* transform = obj->getComponent<TransformComponent>())
				transform->translation = camera->pos;
		}

		RenderDirtyFlags object_dirty =
			RenderDirtyFlagBit(RenderDirtyFlag::Visibility) |
			RenderDirtyFlagBit(RenderDirtyFlag::Transform);
		if (obj->hasComponent<MeshComponent>())
			object_dirty |= RenderDirtyFlagBit(RenderDirtyFlag::Mesh) | RenderDirtyFlagBit(RenderDirtyFlag::Material);
		if (obj->hasComponent<PointLightComponent>() || obj->hasComponent<DirectionalLightComponent>())
			object_dirty |= RenderDirtyFlagBit(RenderDirtyFlag::Light);
		if (obj->hasComponent<CameraComponent>())
			object_dirty |= RenderDirtyFlagBit(RenderDirtyFlag::Camera);
		obj->markRenderDirty(object_dirty);
	}

	if (!mainCameraObject())
		createCamera();
	if (!mainDirectionalLightObject())
		createDirectionalLight();
}

bool Scene::loadProject(const std::string& project_filepath, bool clear_old)
{
	m_current_project_filepath = project_filepath;
	ProjectDTO dto;
	Meta::Serialization::Serializer::loadFromJsonFile(project_filepath, dto);
	applyProjectDTOToScene(dto, clear_old);
	return true;
}

bool Scene::saveProject(const std::string& project_filepath)
{
	m_current_project_filepath = project_filepath;
	ProjectDTO dto = buildProjectDTOFromScene(m_current_project_filepath);
	Meta::Serialization::Serializer::saveToJsonFile(project_filepath, dto);
	return true;
}

void Scene::registerObject(const std::shared_ptr<GObject>& obj, bool mark_created)
{
	if (!obj)
		return;

	m_objects.push_back(obj);
	m_object_by_id[obj->ID().id] = obj;
	refreshRenderObjectCaches(obj->ID());

	if (m_render_signal_bound_object_ids.insert(obj->ID().id).second)
	{
		connect(obj.get(), &obj->renderDirty, this, &Scene::onObjectRenderDirty);
	}

	if (mark_created)
		markRenderDirty(obj->ID(), RenderDirtyFlagBit(RenderDirtyFlag::Created));
}

void Scene::unregisterObject(GObjectID id)
{
	m_object_by_id.erase(id.id);
	removeID(m_directional_light_object_ids, id);
	removeID(m_point_light_object_ids, id);
	if (m_main_camera_object_id == id.id)
		m_main_camera_object_id = 0;
}

void Scene::refreshRenderObjectCaches(GObjectID id)
{
	GObject* object = objectOf(id);
	if (!object)
	{
		removeID(m_directional_light_object_ids, id);
		removeID(m_point_light_object_ids, id);
		return;
	}

	if (object->hasComponent<DirectionalLightComponent>())
		addUniqueID(m_directional_light_object_ids, id);
	else
		removeID(m_directional_light_object_ids, id);

	if (object->hasComponent<PointLightComponent>())
		addUniqueID(m_point_light_object_ids, id);
	else
		removeID(m_point_light_object_ids, id);

	if (object->hasComponent<CameraComponent>() && m_main_camera_object_id == 0)
		m_main_camera_object_id = id.id;
}

GObject* Scene::objectOf(GObjectID id) const
{
	return objectOf(id.id);
}

GObject* Scene::objectOf(int id) const
{
	auto it = m_object_by_id.find(id);
	return it == m_object_by_id.end() ? nullptr : it->second.get();
}

void Scene::removeObject(GObjectID id)
{
	m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(),
		[id](const std::shared_ptr<GObject>& obj)
		{
			return obj && obj->ID() == id;
		}), m_objects.end());
	m_picked_objects.erase(std::remove_if(m_picked_objects.begin(), m_picked_objects.end(),
		[id](const std::shared_ptr<GObject>& obj)
		{
			return obj && obj->ID() == id;
		}), m_picked_objects.end());
	unregisterObject(id);
	markRenderDirty(id, RenderDirtyFlagBit(RenderDirtyFlag::Removed));
}

void Scene::markRenderDirty(GObjectID object_id, RenderDirtyFlags flags)
{
	if (HasRenderDirtyFlag(flags, RenderDirtyFlag::FullResync))
	{
		markFullRenderResync();
		return;
	}
	if (flags == RenderDirtyFlagBit(RenderDirtyFlag::None))
		return;

	auto& change = m_pending_render_changes[object_id.id];
	change.object_id = object_id.id;
	change.flags |= flags;
}

void Scene::markFullRenderResync()
{
	m_full_render_resync = true;
	m_pending_render_changes.clear();
}

std::vector<RenderSceneChange> Scene::consumeRenderChanges()
{
	if (m_full_render_resync)
	{
		m_full_render_resync = false;
		m_pending_render_changes.clear();
		return { RenderSceneChange{ 0, RenderDirtyFlagBit(RenderDirtyFlag::FullResync) } };
	}

	std::vector<RenderSceneChange> changes;
	changes.reserve(m_pending_render_changes.size());
	for (const auto& pair : m_pending_render_changes)
		changes.push_back(pair.second);
	m_pending_render_changes.clear();
	return changes;
}

std::vector<GObjectID> Scene::getPickedObjectIDs() const
{
	std::vector<GObjectID> res(m_picked_objects.size());
	std::transform(m_picked_objects.begin(), m_picked_objects.end(), res.begin(), [](const std::shared_ptr<GObject>& obj) {
		return obj->ID();
		});
	return res;
}

GObject* Scene::mainCameraObject() const
{
	for (const auto& obj : m_objects) {
		if (obj && obj->ID().id == m_main_camera_object_id && obj->hasComponent<CameraComponent>())
			return obj.get();
	}

	for (const auto& obj : m_objects) {
		if (obj && obj->hasComponent<CameraComponent>())
			return obj.get();
	}
	return nullptr;
}

CameraComponent& Scene::getMainCamera() const
{
	GObject* camera_object = mainCameraObject();
	if (camera_object) {
		if (auto* camera = camera_object->getComponent<CameraComponent>())
			return *camera;
	}

	// 正常情况下 Scene 构造和项目读取都会保证主相机存在。
	// 这里的 fallback 只是为了避免 release 环境崩掉，同时让 debug 尽早暴露问题。
	assert(false && "Scene has no main CameraComponent");
	static CameraComponent fallback_camera(nullptr);
	return fallback_camera;
}

std::vector<std::shared_ptr<GObject>> Scene::directionalLightObjects() const
{
	std::vector<std::shared_ptr<GObject>> res;
	for (const GObjectID& id : m_directional_light_object_ids) {
		auto it = m_object_by_id.find(id.id);
		if (it != m_object_by_id.end() && it->second)
			res.push_back(it->second);
	}
	return res;
}

std::vector<std::shared_ptr<GObject>> Scene::pointLightObjects() const
{
	std::vector<std::shared_ptr<GObject>> res;
	for (const GObjectID& id : m_point_light_object_ids) {
		auto it = m_object_by_id.find(id.id);
		if (it != m_object_by_id.end() && it->second)
			res.push_back(it->second);
	}
	return res;
}

GObject* Scene::mainDirectionalLightObject() const
{
	for (const GObjectID& id : m_directional_light_object_ids)
		if (GObject* object = objectOf(id))
			return object;
	return nullptr;
}

int Scene::pointLightCount() const
{
	return static_cast<int>(m_point_light_object_ids.size());
}

void Scene::addObject(std::shared_ptr<GObject> obj)
{
	if (obj && obj->hasComponent<CameraComponent>() && m_main_camera_object_id == 0)
		m_main_camera_object_id = obj->ID().id;
	registerObject(obj);
}

void Scene::onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed)
{
	m_picked_objects.erase(std::remove_if(m_picked_objects.begin(), m_picked_objects.end(), [removed](const std::shared_ptr<GObject>& obj) {
		return std::find(removed.begin(), removed.end(), obj->ID()) != removed.end();
		}), m_picked_objects.end());
	for (const auto& obj : m_objects) {
		if (std::find(added.begin(), added.end(), obj->ID()) != added.end()) {
			m_picked_objects.push_back(obj);
			Logger::debug("Scene::onPickedChanged(), added obj: {} {}", obj->ID().id, obj->name());
		}
	}
}

void Scene::onObjectRenderDirty(GObjectID object_id, RenderDirtyFlags flags)
{
	refreshRenderObjectCaches(object_id);
	markRenderDirty(object_id, flags);
}
