#ifndef ProjectManager_hpp
#define ProjectManager_hpp

#include "Project/Project.hpp"

#include <optional>
#include <string>

class Scene;

class ProjectManager {
public:
	bool openProject(const std::string& project_file);
	bool saveProject(const std::string& project_file) const;

	bool hasProject() const { return m_project.has_value(); }
	const Project* project() const { return m_project ? &(*m_project) : nullptr; }
	const std::string& projectFilepath() const { return m_project_file; }
	void setProjectFilepath(const std::string& project_file);

private:
	std::optional<Project> m_project;
	std::string m_project_file;
};

#endif // !ProjectManager_hpp
