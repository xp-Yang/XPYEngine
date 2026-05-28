#include "Logical/Framework/World/Scene.hpp"

Scene::Scene()
	: m_dirty_tracker()
	, m_registry(m_dirty_tracker)
	, m_selection(m_registry)
	, m_serializer(m_registry, m_dirty_tracker, m_selection)
{
	m_registry.createCamera();
	m_registry.createDirectionalLight();
}

// --- Object creation ---

GObject* Scene::createObject(const std::string& name, bool with_transform)
{
	return m_registry.createObject(name, with_transform);
}

GObject* Scene::createCamera(const std::string& name)
{
	return m_registry.createCamera(name);
}

GObject* Scene::loadModel(const std::string& filepath)
{
	return m_registry.loadModel(filepath);
}

GObject* Scene::createDirectionalLight(const std::string& name)
{
	return m_registry.createDirectionalLight(name);
}

GObject* Scene::createPointLight(const std::string& name)
{
	return m_registry.createPointLight(name);
}

void Scene::removeLastPointLight()
{
	m_registry.removeLastPointLight();
}

// --- Project I/O ---

bool Scene::loadProject(const std::string& project_filepath, bool clear_old)
{
	return m_serializer.loadProject(project_filepath, clear_old);
}

bool Scene::saveProject(const std::string& project_filepath)
{
	return m_serializer.saveProject(project_filepath);
}

const std::string& Scene::currentProjectFilepath() const
{
	return m_serializer.currentProjectFilepath();
}

// --- Selection ---

const std::vector<std::shared_ptr<GObject>>& Scene::getPickedObjects() const
{
	return m_selection.getPickedObjects();
}

std::vector<GObjectID> Scene::getPickedObjectIDs() const
{
	return m_selection.getPickedObjectIDs();
}

// --- Object queries ---

const std::vector<std::shared_ptr<GObject>>& Scene::getObjects() const
{
	return m_registry.getObjects();
}

GObject* Scene::objectOf(GObjectID id) const
{
	return m_registry.objectOf(id);
}

GObject* Scene::objectOf(int id) const
{
	return m_registry.objectOf(id);
}

GObject* Scene::mainCameraObject() const
{
	return m_registry.mainCameraObject();
}

std::vector<std::shared_ptr<GObject>> Scene::directionalLightObjects() const
{
	return m_registry.directionalLightObjects();
}

std::vector<std::shared_ptr<GObject>> Scene::pointLightObjects() const
{
	return m_registry.pointLightObjects();
}

const std::vector<GObjectID>& Scene::directionalLightObjectIDs() const
{
	return m_registry.directionalLightObjectIDs();
}

const std::vector<GObjectID>& Scene::pointLightObjectIDs() const
{
	return m_registry.pointLightObjectIDs();
}

GObject* Scene::mainDirectionalLightObject() const
{
	return m_registry.mainDirectionalLightObject();
}

int Scene::pointLightCount() const
{
	return m_registry.pointLightCount();
}

void Scene::addObject(std::shared_ptr<GObject> obj)
{
	if (obj && obj->hasComponent<CameraComponent>() && m_registry.mainCameraObjectId() == 0)
		m_registry.setMainCameraObjectId(obj->ID().id);
	m_registry.registerObject(obj);
}

CameraComponent& Scene::getMainCamera() const
{
	return m_registry.getMainCamera();
}

void Scene::removeObject(GObjectID id)
{
	m_selection.removeObject(id);
	m_registry.removeObject(id);
}

// --- Dirty tracking ---

void Scene::markDirty(GObjectID object_id, SceneDirtyFlags flags)
{
	m_dirty_tracker.markDirty(object_id, flags);
}

void Scene::markFullResync()
{
	m_dirty_tracker.markFullResync();
}

std::vector<SceneChange> Scene::consumeChanges()
{
	return m_dirty_tracker.consumeChanges();
}

// --- Slots ---

void Scene::onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed)
{
	m_selection.onPickedChanged(std::move(added), std::move(removed));
}

void Scene::onObjectDirty(GObjectID object_id, SceneDirtyFlags flags)
{
	m_registry.onObjectDirty(object_id, flags);
}
