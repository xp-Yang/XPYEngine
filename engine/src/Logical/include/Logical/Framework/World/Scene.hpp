#ifndef Scene_hpp
#define Scene_hpp

#include "Logical/Framework/World/SceneDirtyTracker.hpp"
#include "Logical/Framework/World/SceneObjectRegistry.hpp"
#include "Logical/Framework/World/ProjectSerializer.hpp"
#include "Logical/Framework/World/SelectionManager.hpp"

class Scene {
public:
	Scene();

	// --- Sub-system accessors ---
	SceneObjectRegistry& registry() { return m_registry; }
	const SceneObjectRegistry& registry() const { return m_registry; }
	ProjectSerializer& serializer() { return m_serializer; }
	SelectionManager& selection() { return m_selection; }
	SceneDirtyTracker& dirtyTracker() { return m_dirty_tracker; }

	// --- Facade: object creation (delegates to SceneObjectRegistry) ---
	GObject* createObject(const std::string& name, bool with_transform = true);
	GObject* createCamera(const std::string& name = "Main Camera");
	GObject* loadModel(const std::string& filepath);
	GObject* createDirectionalLight(const std::string& name = "Directional Light");
	GObject* createPointLight(const std::string& name = "Point Light");
	void removeLastPointLight();

	// --- Facade: project I/O (delegates to ProjectSerializer) ---
	bool loadProject(const std::string& project_filepath, bool clear_old = true);
	bool saveProject(const std::string& project_filepath);
	const std::string& currentProjectFilepath() const;

	// --- Facade: selection (delegates to SelectionManager) ---
	const std::vector<std::shared_ptr<GObject>>& getPickedObjects() const;
	std::vector<GObjectID> getPickedObjectIDs() const;

	// --- Facade: object queries (delegates to SceneObjectRegistry) ---
	const std::vector<std::shared_ptr<GObject>>& getObjects() const;
	GObject* objectOf(GObjectID id) const;
	GObject* objectOf(int id) const;
	GObject* mainCameraObject() const;
	std::vector<std::shared_ptr<GObject>> directionalLightObjects() const;
	std::vector<std::shared_ptr<GObject>> pointLightObjects() const;
	const std::vector<GObjectID>& directionalLightObjectIDs() const;
	const std::vector<GObjectID>& pointLightObjectIDs() const;
	GObject* mainDirectionalLightObject() const;
	int pointLightCount() const;
	void addObject(std::shared_ptr<GObject> obj);
	CameraComponent& getMainCamera() const;
	void removeObject(GObjectID id);

	// --- Facade: dirty tracking (delegates to SceneDirtyTracker) ---
	void markDirty(GObjectID object_id, SceneDirtyFlags flags);
	void markFullResync();
	std::vector<SceneChange> consumeChanges();

public slots:
	void onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed);
	void onObjectDirty(GObjectID object_id, SceneDirtyFlags flags);

private:
	SceneDirtyTracker m_dirty_tracker;
	SceneObjectRegistry m_registry;
	SelectionManager m_selection;
	ProjectSerializer m_serializer;
};

#endif // !Scene_hpp
