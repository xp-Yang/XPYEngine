#ifndef Scene_hpp
#define Scene_hpp

#include "Logical/Framework/Object/GObject.hpp"
#include "Logical/Framework/Component/Component.hpp"

#include <unordered_map>
#include <unordered_set>

struct ProjectDTO;

// Object-level change consumed by RenderSystem. This round intentionally keeps
// mesh/material dirtiness coarse; subMesh-level changes can be added later.
struct RenderSceneChange {
	int object_id{ 0 };
	RenderDirtyFlags flags{ RenderDirtyFlagBit(RenderDirtyFlag::None) };
};

class Scene {
public:
	Scene();

	GObject* createObject(const std::string& name, bool with_transform = true);
	GObject* createCamera(const std::string& name = "Main Camera");
	GObject* loadModel(const std::string& filepath);
	GObject* createDirectionalLight(const std::string& name = "Directional Light");
	GObject* createPointLight(const std::string& name = "Point Light");
	void removeLastPointLight();
	bool loadProject(const std::string& project_filepath, bool clear_old = true);
	bool saveProject(const std::string& project_filepath);
	const std::string& currentProjectFilepath() const { return m_current_project_filepath; }
	const std::vector<std::shared_ptr<GObject>>& getPickedObjects() const { return m_picked_objects; }
	std::vector<GObjectID> getPickedObjectIDs() const;
	const std::vector<std::shared_ptr<GObject>>& getObjects() const { return m_objects; }
	GObject* objectOf(GObjectID id) const;
	GObject* objectOf(int id) const;
	GObject* mainCameraObject() const;
	std::vector<std::shared_ptr<GObject>> directionalLightObjects() const;
	std::vector<std::shared_ptr<GObject>> pointLightObjects() const;
	const std::vector<GObjectID>& directionalLightObjectIDs() const { return m_directional_light_object_ids; }
	const std::vector<GObjectID>& pointLightObjectIDs() const { return m_point_light_object_ids; }
	GObject* mainDirectionalLightObject() const;
	int pointLightCount() const;
	void addObject(std::shared_ptr<GObject> obj);
	CameraComponent& getMainCamera() const;
	void removeObject(GObjectID id);

	// Merge render-dirty flags for one object. RenderSystem consumes them once per frame.
	void markRenderDirty(GObjectID object_id, RenderDirtyFlags flags);

	// Force a one-time rebuild of RenderScene from all current Scene objects.
	void markFullRenderResync();

	// Move pending render changes out of Scene and clear the queue.
	std::vector<RenderSceneChange> consumeRenderChanges();

public slots:
	void onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed);
	void onObjectRenderDirty(GObjectID object_id, RenderDirtyFlags flags);

protected:
	ProjectDTO buildProjectDTOFromScene(const std::string& project_filepath);
	void applyProjectDTOToScene(const ProjectDTO& dto, bool clear_old = true);
	void registerObject(const std::shared_ptr<GObject>& obj, bool mark_created = true);
	void unregisterObject(GObjectID id);
	void refreshRenderObjectCaches(GObjectID id);

private:
	std::vector<std::shared_ptr<GObject>> m_objects;
	std::vector<std::shared_ptr<GObject>> m_picked_objects;
	std::unordered_map<int, std::shared_ptr<GObject>> m_object_by_id;
	std::unordered_set<int> m_render_signal_bound_object_ids;
	std::unordered_map<int, RenderSceneChange> m_pending_render_changes;
	std::vector<GObjectID> m_directional_light_object_ids;
	std::vector<GObjectID> m_point_light_object_ids;
	bool m_full_render_resync{ true };
	int m_main_camera_object_id{0};

	std::string m_current_project_filepath;
};

#endif // !Scene_hpp
