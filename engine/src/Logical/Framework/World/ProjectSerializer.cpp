#include "Logical/Framework/World/ProjectSerializer.hpp"
#include "Logical/Framework/World/SceneObjectRegistry.hpp"
#include "Logical/Framework/World/SceneDirtyTracker.hpp"
#include "Logical/Framework/World/SelectionManager.hpp"

#include "Logical/Framework/Component/CameraComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"
#include "Logical/Framework/Component/LightComponent.hpp"
#include "Logical/Framework/Component/TransformComponent.hpp"

#include "AssetManager/DTO.hpp"
#include "Logical/Snapshot/ObjectSnapshotService.hpp"

namespace
{
void convertTexturePaths(MaterialTexturesDTO& textures, const std::string& project_dir, bool make_relative)
{
	auto convert = [&project_dir, make_relative](std::string& path) {
		if (path.empty())
			return;
		path = make_relative ? PathService::tryMakeRelative(project_dir, path) : PathService::join(project_dir, path);
	};

	convert(textures.diffuse);
	convert(textures.specular);
	convert(textures.normal);
	convert(textures.height);
	convert(textures.albedo);
	convert(textures.metallic);
	convert(textures.roughness);
	convert(textures.ao);
}

void convertObjectPaths(ObjectDTO& dto, const std::string& project_dir, bool make_relative)
{
	if (!dto.filepath.empty())
		dto.filepath = make_relative ? PathService::tryMakeRelative(project_dir, dto.filepath) : PathService::join(project_dir, dto.filepath);

	for (MaterialDTO& material : dto.materials)
		convertTexturePaths(material.textures, project_dir, make_relative);
}
}

ProjectSerializer::ProjectSerializer(SceneObjectRegistry& registry,
	SceneDirtyTracker& dirty_tracker, SelectionManager& selection)
	: m_registry(registry)
	, m_dirty_tracker(dirty_tracker)
	, m_selection(selection)
{
}

bool ProjectSerializer::loadProject(const std::string& project_filepath, bool clear_old)
{
	m_current_project_filepath = project_filepath;
	ProjectDTO dto;
	Meta::Serialization::Serializer::loadFromJsonFile(project_filepath, dto);
	applyProjectDTOToScene(dto, clear_old);
	return true;
}

bool ProjectSerializer::saveProject(const std::string& project_filepath)
{
	m_current_project_filepath = project_filepath;
	ProjectDTO dto = buildProjectDTOFromScene(m_current_project_filepath);
	Meta::Serialization::Serializer::saveToJsonFile(project_filepath, dto);
	return true;
}

ProjectDTO ProjectSerializer::buildProjectDTOFromScene(const std::string& project_filepath)
{
	ProjectDTO dto;
	dto.project_name = PathService::getFileName(project_filepath);
	const std::string project_dir = PathService::getDirectory(project_filepath);

	for (const auto& obj_sp : m_registry.getObjects()) {
		if (!obj_sp) continue;
		GObject& obj = *obj_sp;
		const MeshComponent* mc = obj.getComponent<MeshComponent>();
		const CameraComponent* camera = obj.getComponent<CameraComponent>();
		const PointLightComponent* point_light = obj.getComponent<PointLightComponent>();
		const DirectionalLightComponent* directional_light = obj.getComponent<DirectionalLightComponent>();
		if (!mc && !camera && !point_light && !directional_light) continue;

		ObjectDTO obj_dto = Snapshot::ObjectSnapshotService::buildDTOFromObject(obj);
		convertObjectPaths(obj_dto, project_dir, true);
		dto.objects.push_back(std::move(obj_dto));
	}
	return dto;
}

void ProjectSerializer::applyProjectDTOToScene(const ProjectDTO& dto, bool clear_old)
{
	if (clear_old) {
		std::shared_ptr<GObject> main_camera_object;
		GObjectID main_camera_id = m_registry.mainCameraObjectId();
		for (const auto& obj : m_registry.getObjects()) {
			if (obj && obj->ID() == main_camera_id && obj->hasComponent<CameraComponent>()) {
				main_camera_object = obj;
				break;
			}
		}
		m_registry.clear();
		if (main_camera_object)
			m_registry.registerObject(main_camera_object, false);
		else
			m_registry.createCamera();
		m_selection.clear();
		m_dirty_tracker.markFullResync();
	}

	const std::string project_dir = PathService::getDirectory(m_current_project_filepath);

	for (const auto& obj_dto : dto.objects) {
		ObjectDTO runtime_dto = obj_dto;
		convertObjectPaths(runtime_dto, project_dir, false);

		const FileType ft = static_cast<FileType>(runtime_dto.file_type);
		GObject* obj = nullptr;
		if (runtime_dto.has_camera && runtime_dto.filepath.empty())
			obj = m_registry.mainCameraObject() ? m_registry.mainCameraObject()
				: m_registry.createCamera(runtime_dto.name.empty() ? "Main Camera" : runtime_dto.name);
		if (!obj && !runtime_dto.filepath.empty() && (ft == FileType::OBJ || ft == FileType::None))
			obj = m_registry.loadModel(runtime_dto.filepath);
		if (!obj && (runtime_dto.has_point_light || runtime_dto.has_directional_light))
			obj = m_registry.createObject(runtime_dto.name.empty() ? "Light" : runtime_dto.name, runtime_dto.has_point_light);
		if (!obj) continue;

		Snapshot::ObjectSnapshotService::applyDTOToObject(m_registry, *obj, runtime_dto);
		obj->markDirty(Snapshot::ObjectSnapshotService::dirtyFlagsForDTO(runtime_dto));
	}

	if (!m_registry.mainCameraObject())
		m_registry.createCamera();
	if (!m_registry.mainDirectionalLightObject())
		m_registry.createDirectionalLight();
}
