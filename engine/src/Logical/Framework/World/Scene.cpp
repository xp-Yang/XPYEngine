#include "Logical/Framework/World/Scene.hpp"

#include "Logical/Framework/Component/CameraComponent.hpp"
#include "Logical/Framework/Component/MeshComponent.hpp"
#include "Logical/Framework/Component/TransformComponent.hpp"
#include "Logical/Framework/Component/AnimationComponent.hpp"
#include "Logical/Animation/Animation.hpp"

#include "ResourceManager/DTO.hpp"
#include "ResourceManager/ResourceImporter.hpp"

Scene::Scene()
{
	m_light_manager = std::make_shared<LightManager>();
	m_camera = std::make_shared<CameraComponent>(nullptr);
}

GObject* Scene::loadModel(const std::string& filepath)
{
	ResourceImporter model_importer;
	if (!model_importer.load(filepath))
		return nullptr;
	std::vector<int> obj_sub_meshes_idx = model_importer.getSubMeshesIds();
	if (obj_sub_meshes_idx.empty()) {
		//Logger::error("Model datas is empty. File loading fails. Please check if the filepath is all English.");
		return nullptr;
	}
	std::string name = PathService::getFileName(filepath);

#if ENABLE_ECS
	auto& world = ecs::World::get();
	auto entity = world.create_entity();
	world.addComponent<ecs::NameComponent>(entity).name = name;
	world.addComponent<TransformComponent>(entity);
	world.addComponent<ExplosionComponent>(entity);
	auto& renderable = world.addComponent<ecs::RenderableComponent>(entity);
	for (int idx : obj_sub_meshes_idx) {
		renderable.sub_meshes.push_back(Mesh{ idx, MeshFileRef{ MeshFileType::OBJ, filepath}, {}, Mat4(1.0f) });
	}
	auto res = GObject::create(nullptr, entity);
#else
	auto res = GObject::create(nullptr, name);
	res->addComponent<TransformComponent>();
	MeshComponent& mesh = res->addComponent<MeshComponent>();
	mesh.source_filepath = filepath;
	for (int idx : obj_sub_meshes_idx) {
		std::shared_ptr<Mesh> sub_mesh = model_importer.meshOfNode(idx);
		sub_mesh->sub_mesh_idx = idx;
		mesh.sub_meshes.push_back(sub_mesh);
	}
	if (model_importer.hasAnimation()) {
		AnimationComponent& animation = res->addComponent<AnimationComponent>();
		animation.clip_path = filepath;
		animation.clip = std::make_shared<Animation>(filepath, &model_importer);
	}
	m_objects.push_back(std::shared_ptr<GObject>(res));
#endif

	return res;
}

// Scene -> DTO
ProjectDTO Scene::buildProjectDTOFromScene(const std::string& project_filepath)
{
	ProjectDTO dto;
	dto.schema_version = 1;
    dto.project_name = PathService::getFileName(project_filepath);
	const std::string project_dir = PathService::getDirectory(project_filepath);

	for (const auto& obj_sp : this->getObjects()) {
		if (!obj_sp) continue;
		GObject& obj = *obj_sp;
		const TransformComponent* tc = obj.getComponent<TransformComponent>();
		const MeshComponent* mc = obj.getComponent<MeshComponent>();
		if (!mc || mc->source_filepath.empty()) continue;

		ObjectDTO obj_dto;
		obj_dto.name = obj.name();
		obj_dto.visible = obj.visible();
		obj_dto.transform.translation = tc ? tc->translation : Vec3(0.0f);
		obj_dto.transform.rotation = tc ? tc->rotation : Vec3(0.0f);
		obj_dto.transform.scale = tc ? tc->scale : Vec3(1.0f);
		obj_dto.filepath = PathService::tryMakeRelative(project_dir, mc->source_filepath);
		obj_dto.file_type = static_cast<int>(FileType::OBJ);

		for (const auto& sub : mc->sub_meshes) {
			if (!sub) continue;

			SubMeshDTO sm;
			sm.sub_mesh_index = sub->sub_mesh_idx;
			sm.local_transform.translation = sub->translation;
			sm.local_transform.rotation = sub->rotation;
			sm.local_transform.scale = sub->scale;
			obj_dto.sub_meshes.push_back(std::move(sm));

			MaterialDTO mat_dto;
			if (sub->material) {
				mat_dto.alpha = sub->material->alpha;
				mat_dto.base_color_factor = sub->material->base_color_factor;
				mat_dto.metallic_factor = sub->material->metallic_factor;
				mat_dto.roughness_factor = sub->material->roughness_factor;
				mat_dto.ao_factor = sub->material->ao_factor;
				mat_dto.diffuse_factor = sub->material->diffuse_factor;
				mat_dto.specular_factor = sub->material->specular_factor;
				mat_dto.shininess = sub->material->shininess;
				auto rel = [&project_dir](const std::shared_ptr<Texture>& t) {
					return t ? PathService::tryMakeRelative(project_dir, t->texture_filepath) : std::string();
				};
				mat_dto.textures.diffuse = rel(sub->material->diffuse_texture);
				mat_dto.textures.specular = rel(sub->material->specular_texture);
				mat_dto.textures.normal = rel(sub->material->normal_texture);
				mat_dto.textures.height = rel(sub->material->height_texture);
				mat_dto.textures.albedo = rel(sub->material->albedo_texture);
				mat_dto.textures.metallic = rel(sub->material->metallic_texture);
				mat_dto.textures.roughness = rel(sub->material->roughness_texture);
				mat_dto.textures.ao = rel(sub->material->ao_texture);
			}
			obj_dto.materials.push_back(std::move(mat_dto));
		}
		dto.objects.push_back(std::move(obj_dto));
	}
	return dto;
}

// DTO -> Scene
void Scene::applyProjectDTOToScene(const ProjectDTO& dto, bool clear_old)
{
    if (clear_old) {
        this->m_objects.clear();
        this->m_picked_objects.clear();
    }

	const std::string project_dir = PathService::getDirectory(this->m_current_project_filepath);

	for (const auto& obj_dto : dto.objects) {
		if (obj_dto.filepath.empty()) continue;
		const std::string model_abs = PathService::join(project_dir, obj_dto.filepath);
		const FileType ft = static_cast<FileType>(obj_dto.file_type);
		GObject* obj = nullptr;
		if (ft == FileType::OBJ || ft == FileType::None)
			obj = this->loadModel(model_abs);
		if (!obj) continue;

		obj->setName(obj_dto.name.empty() ? obj->name() : obj_dto.name);
		obj->setVisible(obj_dto.visible);
		if (auto* tc = obj->getComponent<TransformComponent>()) {
			tc->translation = obj_dto.transform.translation;
			tc->rotation = obj_dto.transform.rotation;
			tc->scale = obj_dto.transform.scale;
		}
		if (auto* mc = obj->getComponent<MeshComponent>()) {
			mc->source_filepath = model_abs;
			if (!obj_dto.sub_meshes.empty()) {
				std::vector<std::shared_ptr<Mesh>> filtered;
				filtered.reserve(obj_dto.sub_meshes.size());
				for (const auto& sm : obj_dto.sub_meshes) {
					const int expected_idx = sm.sub_mesh_index;
					auto it = std::find_if(mc->sub_meshes.begin(), mc->sub_meshes.end(),
						[expected_idx](const std::shared_ptr<Mesh>& m) { return m && m->sub_mesh_idx == expected_idx; });
					if (it != mc->sub_meshes.end()) filtered.push_back(*it);
				}
				if (!filtered.empty()) mc->sub_meshes = std::move(filtered);
			}
			const size_t n = std::min({ mc->sub_meshes.size(), obj_dto.sub_meshes.size(), obj_dto.materials.size() });
			for (size_t i = 0; i < n; i++) {
				auto& sub = mc->sub_meshes[i];
				if (!sub) continue;
				const TransformDTO& lt = obj_dto.sub_meshes[i].local_transform;
				sub->translation = lt.translation;
				sub->rotation = lt.rotation;
				sub->scale = lt.scale;

				if (!sub->material) sub->material = std::make_shared<Material>();
				const auto& md = obj_dto.materials[i];
				sub->material->alpha = md.alpha;
				sub->material->base_color_factor = md.base_color_factor;
				sub->material->metallic_factor = md.metallic_factor;
				sub->material->roughness_factor = md.roughness_factor;
				sub->material->ao_factor = md.ao_factor;
				sub->material->diffuse_factor = md.diffuse_factor;
				sub->material->specular_factor = md.specular_factor;
				sub->material->shininess = md.shininess;
				auto mktex = [&project_dir](const std::string& rel, TextureType type, bool gamma) -> std::shared_ptr<Texture> {
					return rel.empty() ? nullptr : std::make_shared<Texture>(type, PathService::join(project_dir, rel), gamma);
				};
				sub->material->diffuse_texture = mktex(md.textures.diffuse, TextureType::Diffuse, false);
				sub->material->specular_texture = mktex(md.textures.specular, TextureType::Specular, false);
				sub->material->normal_texture = mktex(md.textures.normal, TextureType::Normal, false);
				sub->material->height_texture = mktex(md.textures.height, TextureType::Height, false);
				sub->material->albedo_texture = mktex(md.textures.albedo, TextureType::Albedo, true);
				sub->material->metallic_texture = mktex(md.textures.metallic, TextureType::Metallic, false);
				sub->material->roughness_texture = mktex(md.textures.roughness, TextureType::Roughness, false);
				sub->material->ao_texture = mktex(md.textures.ao, TextureType::AO, false);
				sub->material->markDirty();
			}
		}
	}
}

bool Scene::loadProject(const std::string& project_filepath, bool clear_old)
{
	m_current_project_filepath = project_filepath;
	ProjectDTO dto;
	Meta::Serialization::Serializer::loadFromJsonFile(project_filepath, dto);
	applyProjectDTOToScene(dto, clear_old);
	return true;
}

bool Scene::saveProject(const std::string& project_filepath)
{
	m_current_project_filepath = project_filepath;
	ProjectDTO dto = buildProjectDTOFromScene(m_current_project_filepath);
	Meta::Serialization::Serializer::saveToJsonFile(project_filepath, dto);
	return true;
}

std::vector<GObjectID> Scene::getPickedObjectIDs() const
{
	std::vector<GObjectID> res(m_picked_objects.size());
	std::transform(m_picked_objects.begin(), m_picked_objects.end(), res.begin(), [](const std::shared_ptr<GObject>& obj) {
		return obj->ID();
		});
	return res;
}

void Scene::addObject(std::shared_ptr<GObject> obj)
{
	m_objects.push_back(std::shared_ptr<GObject>(obj));
}

void Scene::onPickedChanged(std::vector<GObjectID> added, std::vector<GObjectID> removed)
{
	m_picked_light.reset();
	m_picked_objects.erase(std::remove_if(m_picked_objects.begin(), m_picked_objects.end(), [removed](const std::shared_ptr<GObject>& obj) {
		return std::find(removed.begin(), removed.end(), obj->ID()) != removed.end();
		}), m_picked_objects.end());
	for (const auto& obj : m_objects) {
		if (std::find(added.begin(), added.end(), obj->ID()) != added.end()) {
			m_picked_objects.push_back(obj);
			Logger::debug("Scene::onPickedChanged(), added obj: {} {}", obj->ID().id, obj->name());
		}
	}
}

void Scene::onPickedChanged(LightID light_id)
{
	m_picked_light.reset();
	m_picked_objects.clear();
	const std::vector<std::shared_ptr<Light>>& lights = m_light_manager->lights();
	auto it = std::find_if(lights.begin(), lights.end(), [light_id](const std::shared_ptr<Light>& light) {
		return light->ID() == light_id;
		});
	if (it != lights.end()) {
		m_picked_light = *it;
		Logger::debug("Scene::onPickedChanged(), light: {} {}", m_picked_light->ID().id, m_picked_light->name());
	}
}
