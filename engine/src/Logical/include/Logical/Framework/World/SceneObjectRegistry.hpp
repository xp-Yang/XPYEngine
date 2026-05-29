#ifndef SceneObjectRegistry_hpp
#define SceneObjectRegistry_hpp

#include "Logical/Framework/Object/GObject.hpp"
#include "Logical/Framework/World/SceneDirtyTracker.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

class SceneObjectRegistry {
public:
	explicit SceneObjectRegistry(SceneDirtyTracker& dirty_tracker);

	// --- Object creation ---
	GObject* createObject(const std::string& name, bool with_transform = true);
	GObject* createCamera(const std::string& name = "Main Camera");
	GObject* createDirectionalLight(const std::string& name = "Directional Light");
	GObject* createPointLight(const std::string& name = "Point Light");
	void removeLastPointLight();
	GObject* loadModel(const std::string& filepath);

	// --- Object registration ---
	void registerObject(const std::shared_ptr<GObject>& obj, bool mark_created = true);
	void unregisterObject(GObjectID id);
	void removeObject(GObjectID id);
	void clear();

	// --- Queries ---
	const std::vector<std::shared_ptr<GObject>>& getObjects() const { return m_objects; }
	GObject* objectOf(GObjectID id) const;
	GObject* objectOf(int id) const;

	GObject* mainCameraObject() const;
	CameraComponent& getMainCamera() const;
	GObjectID mainCameraObjectId() const { return m_main_camera_object_id; }
	void setMainCameraObjectId(GObjectID id) { m_main_camera_object_id = id; }

	std::vector<std::shared_ptr<GObject>> directionalLightObjects() const;
	std::vector<std::shared_ptr<GObject>> pointLightObjects() const;
	const std::vector<GObjectID>& directionalLightObjectIDs() const { return m_directional_light_object_ids; }
	const std::vector<GObjectID>& pointLightObjectIDs() const { return m_point_light_object_ids; }
	GObject* mainDirectionalLightObject() const;
	int pointLightCount() const;

	void refreshCaches(GObjectID id);
	void onObjectDirty(GObjectID object_id, SceneDirtyFlags flags);

private:
	SceneDirtyTracker& m_dirty_tracker;

	std::vector<std::shared_ptr<GObject>> m_objects;
	std::unordered_map<GObjectID, std::shared_ptr<GObject>> m_object_by_id;
	std::unordered_set<GObjectID> m_signal_bound_object_ids;
	std::vector<GObjectID> m_directional_light_object_ids;
	std::vector<GObjectID> m_point_light_object_ids;
	GObjectID m_main_camera_object_id{};
};

#endif // !SceneObjectRegistry_hpp
