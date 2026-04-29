#include "Project/ProjectManager.hpp"

#include "Base/Utils/PathService.hpp"

#include <json11.hpp>

#include <fstream>
#include <sstream>

static std::string dir_of(const std::string& filepath)
{
	std::string p = PathService::normalize(filepath);
	const size_t pos = p.find_last_of('/');
	return (pos == std::string::npos) ? std::string() : p.substr(0, pos);
}

bool ProjectManager::openProject(const std::string& project_file)
{
	m_project_file = PathService::normalize(project_file);

	std::ifstream fin(m_project_file);
	if (!fin)
		return false;
	std::stringstream buffer;
	buffer << fin.rdbuf();

	std::string error;
	const auto json = json11::Json::parse(buffer.str(), error);
	if (!error.empty())
		return false;

	Project p;
	p.schema_version = (int)json["schema_version"].number_value();
	if (p.schema_version == 0)
		p.schema_version = 1;
	// Forward-compatible: ignore unknown fields.
	p.project_name = json["project_name"].string_value();
	if (p.project_name.empty())
		p.project_name = "XPYProject";
	p.project_root_abs = dir_of(m_project_file);

	m_project = std::move(p);
	return true;
}

bool ProjectManager::saveProject(const std::string& project_file) const
{
	if (!m_project)
		return false;

	json11::Json::object root;
	root["schema_version"] = m_project->schema_version;
	root["project_name"] = m_project->project_name;

	std::ofstream fout(PathService::normalize(project_file));
	if (!fout)
		return false;
	fout << json11::Json(root).dump();
	fout.flush();
	return (bool)fout;
}

void ProjectManager::setProjectFilepath(const std::string& project_file)
{
	m_project_file = PathService::normalize(project_file);
	if (m_project) {
		m_project->project_root_abs = dir_of(m_project_file);
	}
}

