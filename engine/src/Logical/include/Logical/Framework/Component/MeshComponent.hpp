#ifndef MeshComponent_hpp
#define MeshComponent_hpp

#include "Logical/Framework/Component/Component.hpp"
#include "AssetManager/Mesh.hpp"

struct MeshComponent : public Component {
	MeshComponent(GObject* parent) : Component(parent) {}

	// Source model file used to build sub_meshes.
	std::string source_filepath;
	std::vector<std::shared_ptr<Mesh>> sub_meshes;
	bool staticShadowCaster{ true };
};

#endif // !MeshComponent_hpp
