#include "cubetest.hpp"
#include "Engine.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include "Logical/Framework/Object/GObject.hpp"
#include "Logical/Mesh.hpp"
#include "Logical/Material.hpp"

void Cubetest::init() {
#if ENABLE_ECS
	auto& world = ecs::World::get();
	auto root_entity = world.create_entity();
	world.addComponent<ecs::NameComponent>(root_entity).name = "Root";

	//initMainCamera
	auto camera = world.create_entity();
	world.addComponent<ecs::NameComponent>(camera).name = "Main Camera";
	auto& camera_component = world.addComponent<CameraComponent>(camera);

	//createSkybox
	auto skybox_entity = world.create_entity();
	world.addComponent<ecs::NameComponent>(skybox_entity).name = "Skybox";
	world.addComponent<ecs::TransformComponent>(skybox_entity);
	auto& skybox_com = world.addComponent<ecs::SkyboxComponent>(skybox_entity);
	skybox_com.cube_texture = CubeTexture(
		RESOURCE_DIRECTORY + "/images/skybox/right.jpg",
		RESOURCE_DIRECTORY + "/images/skybox/left.jpg",
		RESOURCE_DIRECTORY + "/images/skybox/top.jpg",
		RESOURCE_DIRECTORY + "/images/skybox/bottom.jpg",
		RESOURCE_DIRECTORY + "/images/skybox/front.jpg",
		RESOURCE_DIRECTORY + "/images/skybox/back.jpg");
	SubMesh sub_mesh;
	sub_mesh.mesh_file_ref = { MeshFileType::CustomCube, "" };
	skybox_com.mesh = sub_mesh;

	//createDirectionalLight
	auto dir_light_entity = world.create_entity();
	auto directional_light_node = GObject::create(nullptr, dir_light_entity);
	world.addComponent<ecs::NameComponent>(dir_light_entity).name = "Directional Light";
	auto& dir_light_properties = world.addComponent<ecs::DirectionalLightComponent>(dir_light_entity);
	dir_light_properties.luminousColor = Color4(2.0f);
	dir_light_properties.direction = { 15.0f, -30.0f, 15.0f };

	size_t point_lights_count = 2;
	auto root_point_lights_entity = world.create_entity();
	world.addComponent<ecs::NameComponent>(root_point_lights_entity).name = "Point Lights";
	auto m_root_point_light_object = GObject::create(nullptr, root_point_lights_entity);
	for (int i = 0; i < point_lights_count; i++) {
		auto point_light_entity = world.create_entity();
		auto point_light_node = GObject::create(m_root_point_light_object, point_light_entity);
		world.addComponent<ecs::NameComponent>(point_light_entity).name = std::string("Point Light ") + std::to_string(i);
		auto& point_light_transform = world.addComponent<ecs::TransformComponent>(point_light_entity);
		double r1 = random(-15.0f, 15.0f);
		double r2 = random(3.0f, 10.0f);
		double r3 = random(-15.0f, 15.0f);
		point_light_transform.translation = { r1, r2, r3 };
		point_light_transform.scale = Vec3(random(0.1f, 0.3f));
		auto& point_light_com = world.addComponent<ecs::PointLightComponent>(point_light_entity);
		point_light_com.radius = (point_light_transform.scale[0]) * 100.0f;
		point_light_com.luminousColor = i == 0 ? Color4(2.0f, 2.0f, 2.0f, 1.0f) : Color4{ randomUnit(), randomUnit(), randomUnit(), 1.0f };
		SubMesh sub_mesh;
		sub_mesh.mesh_file_ref = { MeshFileType::CustomSphere, "" };
		point_light_com.mesh = sub_mesh;
	}

	auto updateSpheresPosition = [&](int sphere_count)
	{
		int one_sequence_max_num = 16;
		int max_cols = sqrt(one_sequence_max_num);
		int max_rows = max_cols;
		int sequences = sphere_count / one_sequence_max_num + 1;
		int cols = std::ceil(sqrt((sphere_count - 1) % one_sequence_max_num + 1));
		int rows = cols;

		float space = 3.0f;
		int i = 0;
		for (auto sphere_obj : m_root_sphere_object->children()) {
			auto& sphere_translation = world.getComponent<TransformComponent>(sphere_obj->entity())->translation;
			int sequence = i / one_sequence_max_num;
			if (sequence == sequences - 1) {
				int j = i - sequence * one_sequence_max_num;
				int col = j % cols;
				int row = (j / cols) % rows;
				sphere_translation = { space * col, 1.0f + space * row, space * sequence };
			}
			else {
				int col = i % max_cols;
				int row = (i / max_cols) % max_rows;
				sphere_translation = { space * col, 1.0f + space * row, space * sequence };
			}
			//float theta = (2 * Core::MathConstant::PI / 25.0f) * m_test_sphere_count;
			//sphere_translation = { 10.0f * cos(theta), 1.0f , 10.0f * sin(theta) };
			i++;
		}
	};

	size_t spheres_count = 64;
	auto root_sphere_entity = world.create_entity();
	world.addComponent<ecs::NameComponent>(root_sphere_entity).name = "Spheres";
	m_root_sphere_object = GObject::create(nullptr, root_sphere_entity);
	for (int i = 0; i < spheres_count; i++) {
		auto sphere_entity = world.create_entity();
		auto sphere_node = GObject::create(m_root_sphere_object, sphere_entity);
		world.addComponent<ecs::NameComponent>(sphere_entity).name = std::string("Sphere") + std::to_string(i);
		world.addComponent<TransformComponent>(sphere_entity);

		auto& sphere_renderable = world.addComponent<ecs::RenderableComponent>(sphere_entity);
		SubMesh sub_mesh;
		sub_mesh.mesh_file_ref = { MeshFileType::CustomSphere, "" };
		sub_mesh.material.albedo = Vec3(1.0f, 1.0f, 1.0f);
		sub_mesh.material.metallic = 1.0;
		sub_mesh.material.roughness = (1.0f / 64) * (i + 1);
		sub_mesh.material.ao = 0.01;
		sphere_renderable.sub_meshes.push_back(sub_mesh);
		//world.addComponent<ExplosionComponent>(sphere_entity);

		updateSpheresPosition(i);

		//Meta::Serialization::Serializer::saveToJsonFile(asset_dir + "/sphere.json", world.getComponent<TransformComponent>(sphere_entity));
		////TransformComponent trans;
		////Meta::Serialization::Serializer::loadFromJsonFile(asset_dir + "/sphere.json", &trans);
		//Meta::Serialization::Serializer::write(world.getComponent<TransformComponent>(sphere_entity));
	}

	//createPlaneGround
	auto ground_entity = world.create_entity();
	auto ground_node = GObject::create(nullptr, ground_entity);
	world.addComponent<ecs::NameComponent>(ground_entity).name = "Gound";
	auto& ground_transform = world.addComponent<ecs::TransformComponent>(ground_entity);
	ground_transform.scale = Vec3(1.0f);
	auto& ground_renderable = world.addComponent<ecs::RenderableComponent>(ground_entity);
	SubMesh sub_mesh;
	sub_mesh.mesh_file_ref = { MeshFileType::CustomGround, "" };
	std::shared_ptr<Material> ground_material = Material::create_default_material();
	sub_mesh.material.albedo = Vec3(1.0f, 1.0f, 1.0f);
	sub_mesh.material.metallic = 0.0f;
	sub_mesh.material.roughness = 1.0f;
	sub_mesh.material.ao = 0.01;
	ground_renderable.sub_meshes.push_back(sub_mesh);

	loadModel(RESOURCE_DIRECTORY + "/model/nanosuit/nanosuit.obj");

	GObject* bunny_obj = loadModel(RESOURCE_DIRECTORY + "/model/bunny.obj");
	auto bunny_transform = world.getComponent<ecs::TransformComponent>(bunny_obj->entity());
	bunny_transform->scale = Vec3(40.0f);
	bunny_transform->translation = Vec3(-10.0f, 0.0f, 0.0f);

	//loadModel(RESOURCE_DIRECTORY + "/model/dragon.obj");

#endif // ENABLE_ECS

	auto& engine = Engine::get();
	auto scene = engine.Scene();

	{
		//GObject* plane_obj = GObject::create(nullptr, "Ground");
		//MeshComponent& plane_mesh = plane_obj->addComponent<MeshComponent>();
		//std::shared_ptr<Mesh> plane_sub_mesh = Mesh::create_complex_quad_mesh(Vec2(10.0f));
		//std::shared_ptr<Material> plane_material = Material::create_complete_default_material();
		//plane_sub_mesh->material = plane_material;
		//plane_mesh.sub_meshes.push_back(plane_sub_mesh);
		//TransformComponent& plane_transform = plane_obj->addComponent<TransformComponent>();
		//plane_transform.scale = Vec3(50.0f, 1.f, 50.0f);
		//scene->addObject(std::shared_ptr<GObject>(plane_obj));

		GObject* ground_obj = scene->loadModel(RESOURCE_DIRECTORY + "/model/basic/cube.obj");
        ground_obj->setName("Ground");
		TransformComponent* transform = ground_obj->getComponent<TransformComponent>();
		transform->scale = Vec3(30.0f, 0.001f, 30.0f);
		std::shared_ptr<Material> ground_material = Material::create_complete_default_material();
		ground_obj->getComponent<MeshComponent>()->sub_meshes[0]->material = ground_material;
	}

	//{
	//	size_t cubes_count = 16;
	//	size_t row_count = std::sqrt(cubes_count);
	//	size_t col_count = cubes_count / row_count;
	//	for (int i = 0; i < cubes_count; i++) {
	//		GObject* cube_obj = scene->loadModel(RESOURCE_DIRECTORY + "/model/basic/cube.obj");
	//		TransformComponent* transform = cube_obj->getComponent<TransformComponent>();
	//		transform->translation = { 1.5f * (i % col_count), 0.5f + 1.5f * (i / row_count), 0.0f };
	//		std::shared_ptr<Material> cube_material = Material::create_complete_default_material();
	//		cube_obj->getComponent<MeshComponent>()->sub_meshes[0]->material = cube_material;
	//	}
	//}

	//{
	//	size_t spheres_count = 16;
	//	size_t s_row_count = std::sqrt(spheres_count);
	//	size_t s_col_count = spheres_count / s_row_count;
	//	for (int i = 0; i < spheres_count; i++) {
	//		GObject* sphere_obj = scene->loadModel(RESOURCE_DIRECTORY + "/model/basic/sphere.obj");
	//		TransformComponent* transform = sphere_obj->getComponent<TransformComponent>();
	//		transform->translation = { 1.5f * (i % s_col_count), 0.5f + 1.5f * (i / s_row_count), 0.0f };
	//		std::shared_ptr<Material> sphere_material = Material::create_complete_default_material();
	//		sphere_obj->getComponent<MeshComponent>()->sub_meshes[0]->material = sphere_material;
	//	}
	//}

	{
		//GObject* nano_suit = scene->loadModel(RESOURCE_DIRECTORY + "/model/nanosuit/nanosuit.obj");
		//nano_suit->getComponent<TransformComponent>()->scale = Vec3(0.3f);

		//GObject* vampire = loadModel(RESOURCE_DIRECTORY + "/model/vampire/dancing_vampire.dae");
		//vampire->getComponent<TransformComponent>()->scale = Vec3(0.02f);
		//vampire->getComponent<TransformComponent>()->translation = Vec3(5.0f, 0.0f, 0.0f);

		//GObject* bunny_obj = loadModel(RESOURCE_DIRECTORY + "/model/bunny.obj");
		//auto bunny_transform = bunny_obj->getComponent<TransformComponent>();
		//bunny_transform->scale = Vec3(75.0f);
	}
}
