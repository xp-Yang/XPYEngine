#include "Logical/Framework/World/SceneObjectRegistry.hpp"

#include "Logical/Framework/Component/CameraComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"
#include "Logical/Framework/Component/LightComponent.hpp"
#include "Logical/Framework/Component/TransformComponent.hpp"
#include "Logical/Framework/Component/AnimationComponent.hpp"
#include "Logical/Animation/Animation.hpp"

#include "AssetManager/ModelImporter.hpp"

#include <algorithm>
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

SceneObjectRegistry::SceneObjectRegistry(SceneDirtyTracker& dirty_tracker)
	: m_dirty_tracker(dirty_tracker)
{
}

// --- Object creation ---

GObject* SceneObjectRegistry::createObject(const std::string& name, bool with_transform)
{
	auto obj = GObject::create(nullptr, name);
	if (with_transform)
		obj->addComponent<TransformComponent>();
	registerObject(std::shared_ptr<GObject>(obj));
	return obj;
}

GObject* SceneObjectRegistry::createCamera(const std::string& name)
{
	GObject* obj = createObject(name);
	auto& camera = obj->addComponent<CameraComponent>();
	camera.refreshView();
	camera.refreshProjection();

	if (auto* transform = obj->getComponent<TransformComponent>())
		transform->translation = camera.pos;

	m_main_camera_object_id = obj->ID();
	obj->markDirty(
		SceneDirtyFlagBit(SceneDirtyFlag::Transform) |
		SceneDirtyFlagBit(SceneDirtyFlag::Camera));
	return obj;
}

GObject* SceneObjectRegistry::createDirectionalLight(const std::string& name)
{
	GObject* obj = createObject(name, false);
	auto& light = obj->addComponent<DirectionalLightComponent>();
	light.luminousColor = Color3(1.0f);
	light.direction = { 15.0f, -30.0f, 15.0f };
	obj->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Light));
	return obj;
}

GObject* SceneObjectRegistry::createPointLight(const std::string& name)
{
	GObject* obj = createObject(name);
	auto& transform = *obj->getComponent<TransformComponent>();
	transform.translation = {
		static_cast<float>(Math::random(-15.0f, 15.0f)),
		static_cast<float>(Math::random(1.0f, 30.0f)),
		static_cast<float>(Math::random(-15.0f, 15.0f))
	};
	obj->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Transform));

	auto& light = obj->addComponent<PointLightComponent>();
	light.radius = 30.0f;
	light.luminousColor = Color3(
		static_cast<float>(Math::randomUnit()),
		static_cast<float>(Math::randomUnit()),
		static_cast<float>(Math::randomUnit()));
	obj->markDirty(SceneDirtyFlagBit(SceneDirtyFlag::Light));
	return obj;
}

void SceneObjectRegistry::removeLastPointLight()
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

GObject* SceneObjectRegistry::loadModel(const std::string& filepath)
{
	ModelImporter model_importer;
	if (!model_importer.load(filepath))
		return nullptr;
	std::vector<int> obj_sub_meshes_idx = model_importer.getSubMeshesIds();
	if (obj_sub_meshes_idx.empty()) {
		return nullptr;
	}
	std::string name = PathService::getFileName(filepath);

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

	return res;
}

// --- Object registration ---

void SceneObjectRegistry::registerObject(const std::shared_ptr<GObject>& obj, bool mark_created)
{
	if (!obj)
		return;

	m_objects.push_back(obj);
	m_object_by_id[obj->ID()] = obj;
	refreshCaches(obj->ID());

	if (m_signal_bound_object_ids.insert(obj->ID()).second)
	{
		connect(obj.get(), &obj->dirty, this, &SceneObjectRegistry::onObjectDirty);
	}

	if (mark_created)
		m_dirty_tracker.markDirty(obj->ID(), SceneDirtyFlagBit(SceneDirtyFlag::Created));
}

void SceneObjectRegistry::unregisterObject(GObjectID id)
{
	m_object_by_id.erase(id);
	m_signal_bound_object_ids.erase(id);
	removeID(m_directional_light_object_ids, id);
	removeID(m_point_light_object_ids, id);
	if (m_main_camera_object_id == id)
		m_main_camera_object_id = {};
}

void SceneObjectRegistry::removeObject(GObjectID id)
{
	m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(),
		[id](const std::shared_ptr<GObject>& obj)
		{
			return obj && obj->ID() == id;
		}), m_objects.end());
	unregisterObject(id);
	m_dirty_tracker.markDirty(id, SceneDirtyFlagBit(SceneDirtyFlag::Removed));
}

void SceneObjectRegistry::clear()
{
	m_objects.clear();
	m_object_by_id.clear();
	m_signal_bound_object_ids.clear();
	m_directional_light_object_ids.clear();
	m_point_light_object_ids.clear();
}

// --- Queries ---

GObject* SceneObjectRegistry::objectOf(GObjectID id) const
{
	auto it = m_object_by_id.find(id);
	return it == m_object_by_id.end() ? nullptr : it->second.get();
}

GObject* SceneObjectRegistry::objectOf(int id) const
{
	return objectOf(GObjectID(id));
}

GObject* SceneObjectRegistry::mainCameraObject() const
{
	for (const auto& obj : m_objects) {
		if (obj && obj->ID() == m_main_camera_object_id && obj->hasComponent<CameraComponent>())
			return obj.get();
	}

	for (const auto& obj : m_objects) {
		if (obj && obj->hasComponent<CameraComponent>())
			return obj.get();
	}
	return nullptr;
}

CameraComponent& SceneObjectRegistry::getMainCamera() const
{
	GObject* camera_object = mainCameraObject();
	if (camera_object) {
		if (auto* camera = camera_object->getComponent<CameraComponent>())
			return *camera;
	}

	assert(false && "Scene has no main CameraComponent");
	static CameraComponent fallback_camera(nullptr);
	return fallback_camera;
}

std::vector<std::shared_ptr<GObject>> SceneObjectRegistry::directionalLightObjects() const
{
	std::vector<std::shared_ptr<GObject>> res;
	for (const GObjectID& id : m_directional_light_object_ids) {
		auto it = m_object_by_id.find(id);
		if (it != m_object_by_id.end() && it->second)
			res.push_back(it->second);
	}
	return res;
}

std::vector<std::shared_ptr<GObject>> SceneObjectRegistry::pointLightObjects() const
{
	std::vector<std::shared_ptr<GObject>> res;
	for (const GObjectID& id : m_point_light_object_ids) {
		auto it = m_object_by_id.find(id);
		if (it != m_object_by_id.end() && it->second)
			res.push_back(it->second);
	}
	return res;
}

GObject* SceneObjectRegistry::mainDirectionalLightObject() const
{
	for (const GObjectID& id : m_directional_light_object_ids)
		if (GObject* object = objectOf(id))
			return object;
	return nullptr;
}

int SceneObjectRegistry::pointLightCount() const
{
	return static_cast<int>(m_point_light_object_ids.size());
}

void SceneObjectRegistry::refreshCaches(GObjectID id)
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

	if (object->hasComponent<CameraComponent>() && !m_main_camera_object_id.isValid())
		m_main_camera_object_id = id;
}

void SceneObjectRegistry::onObjectDirty(GObjectID object_id, SceneDirtyFlags flags)
{
	refreshCaches(object_id);
	m_dirty_tracker.markDirty(object_id, flags);
}
