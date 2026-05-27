#ifndef ModelImporter_hpp
#define ModelImporter_hpp

// import files in formats other than XPYEngin Asset file

#include "Base/Common.hpp"
#include "AssetManager/Mesh.hpp"

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;
namespace Assimp {
	class Importer;
}

struct Mesh;
struct Material;

struct BoneInfo
{
	/*id is index in finalBoneMatrices*/
	int id;
	/*offset matrix transforms vertex from model space to bone space*/
	Mat4 offset;
};

class ModelImporter {
public:
    ModelImporter() = default;
	~ModelImporter();
	bool load(const std::string& obj_file_path);
	std::shared_ptr<Mesh> meshOfNode(int ai_mesh_idx);
	std::shared_ptr<Material> materialOfNode(int ai_mesh_idx);
	std::vector<int> getSubMeshesIds() const;
	bool hasAnimation() const;
	const std::map<std::string, BoneInfo>& getBoneInfoMap() const { return m_BoneInfoMap; }
	int getBoneCount() const { return m_BoneCounter; }

protected:
	std::vector<aiMesh*> collect_ai_meshes();
	std::shared_ptr<Mesh> load_sub_mesh_data(aiMesh* mesh);
	std::shared_ptr<Material> load_material(aiMaterial* material);
	void extractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh);
	void setVertexBoneData(Vertex& vertex, int bone_id, float weight) const;

private:
	static std::unordered_map<std::string, Assimp::Importer*> m_importers;

	const aiScene* m_scene{ nullptr };
	std::map<std::string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter;

	std::string m_obj_filepath;
	std::string m_directory;
};

#endif
