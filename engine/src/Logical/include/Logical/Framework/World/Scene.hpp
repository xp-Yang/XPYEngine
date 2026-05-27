#ifndef Scene_hpp
#define Scene_hpp

#include "Logical/Framework/Object/GObject.hpp"
#include "Logical/Framework/Component/Component.hpp"

struct ProjectDTO;
class Scene {
public:
	Scene();

	GObject* createObject(const std::string& name);
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
	std::vector<std::shared_ptr<GObject>> directionalLightObjects() const;
	std::vector<std::shared_ptr<GObject>> pointLightObjects() const;
	GObject* mainDirectionalLightObject() const;
	int pointLightCount() const;
	void addObject(std::shared_ptr<GObject> obj);
	CameraComponent& getMainCamera() const { return *m_camera; }

public slots:
	void onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed);

protected:
	ProjectDTO buildProjectDTOFromScene(const std::string& project_filepath);
	void applyProjectDTOToScene(const ProjectDTO& dto, bool clear_old = true);

private:
	std::vector<std::shared_ptr<GObject>> m_objects;
	std::vector<std::shared_ptr<GObject>> m_picked_objects;
	std::shared_ptr<CameraComponent> m_camera;

	std::string m_current_project_filepath;
};

#endif // !Scene_hpp
