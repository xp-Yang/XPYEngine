#ifndef Scene_hpp
#define Scene_hpp

#include "Logical/Framework/Object/GObject.hpp"
#include "Logical/Framework/Component/Component.hpp"
#include "Logical/Framework/World/LightManager.hpp"

class GcodeViewer;
class GCodeProcessorResult;
struct ProjectDTO;
class Scene {
public:
	Scene();

	GObject* loadModel(const std::string& filepath);
	GCodeProcessorResult loadGcodeFile(const std::string& filepath);
	bool loadProject(const std::string& project_filepath);
	bool saveProject(const std::string& project_filepath);
	const std::string& currentProjectFilepath() const { return m_current_project_filepath; }
	const std::vector<std::shared_ptr<GObject>>& getPickedObjects() const { return m_picked_objects; }
	std::vector<GObjectID> getPickedObjectIDs() const;
	std::shared_ptr<Light> getPickedLight() const { return m_picked_light; }
	const std::vector<std::shared_ptr<GObject>>& getObjects() const { return m_objects; }
	void addObject(std::shared_ptr<GObject> obj);
	std::shared_ptr<LightManager> getLightManager() const { return m_light_manager; }
	CameraComponent& getMainCamera() const { return *m_camera; }
	std::shared_ptr<GcodeViewer> gcodeViewer() const;

public slots:
	void onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed);
	void onPickedChanged(LightID light_id);

protected:
	friend ProjectDTO buildProjectDTOFromScene(const Scene& scene, const std::string& project_filepath);
	friend void applyProjectDTOToScene(const ProjectDTO& dto, Scene& scene);

private:
	std::vector<std::shared_ptr<GObject>> m_objects;
	std::shared_ptr<LightManager> m_light_manager;
	std::vector<std::shared_ptr<GObject>> m_picked_objects;
	std::shared_ptr<Light> m_picked_light;
	std::shared_ptr<CameraComponent> m_camera;
	std::shared_ptr<GcodeViewer> m_gcode_viewer;

	std::string m_current_project_filepath;
};

#endif // !Scene_hpp
