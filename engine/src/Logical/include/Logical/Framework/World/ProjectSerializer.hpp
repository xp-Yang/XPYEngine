#ifndef ProjectSerializer_hpp
#define ProjectSerializer_hpp

#include <string>

struct ProjectDTO;
class SceneObjectRegistry;
class SceneDirtyTracker;
class SelectionManager;

class ProjectSerializer {
public:
	ProjectSerializer(SceneObjectRegistry& registry,
		SceneDirtyTracker& dirty_tracker, SelectionManager& selection);

	bool loadProject(const std::string& project_filepath, bool clear_old = true);
	bool saveProject(const std::string& project_filepath);
	const std::string& currentProjectFilepath() const { return m_current_project_filepath; }

private:
	ProjectDTO buildProjectDTOFromScene(const std::string& project_filepath);
	void applyProjectDTOToScene(const ProjectDTO& dto, bool clear_old);

	SceneObjectRegistry& m_registry;
	SceneDirtyTracker& m_dirty_tracker;
	SelectionManager& m_selection;
	std::string m_current_project_filepath;
};

#endif // !ProjectSerializer_hpp
