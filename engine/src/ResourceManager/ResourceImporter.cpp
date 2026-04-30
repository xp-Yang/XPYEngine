#include "ResourceImporter.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Logical/Mesh.hpp"

std::unordered_map<std::string, Assimp::Importer *> ResourceImporter::m_importers;

namespace {
Mat4 toMat4(const aiMatrix4x4 &mat)
{
    Mat4 res(1.0f);
    res[0][0] = mat.a1;
    res[1][0] = mat.a2;
    res[2][0] = mat.a3;
    res[3][0] = mat.a4;
    res[0][1] = mat.b1;
    res[1][1] = mat.b2;
    res[2][1] = mat.b3;
    res[3][1] = mat.b4;
    res[0][2] = mat.c1;
    res[1][2] = mat.c2;
    res[2][2] = mat.c3;
    res[3][2] = mat.c4;
    res[0][3] = mat.d1;
    res[1][3] = mat.d2;
    res[2][3] = mat.d3;
    res[3][3] = mat.d4;
    return res;
}
} // namespace

ResourceImporter::~ResourceImporter()
{
}

bool ResourceImporter::load(const std::string &file_path)
{
    m_obj_filepath = file_path;
    m_directory = file_path.substr(0, file_path.find_last_of("/\\"));
    m_BoneInfoMap.clear();
    m_BoneCounter = 0;

    if (ResourceImporter::m_importers.find(file_path) != ResourceImporter::m_importers.end())
    {
        m_scene = ResourceImporter::m_importers.at(file_path)->GetScene();
    }
    else
    {
        auto importer = new Assimp::Importer();
        m_scene = importer->ReadFile(file_path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
        if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode)
        {
            auto error_str = importer->GetErrorString();
            delete importer;
            return false;
        }
        ResourceImporter::m_importers.insert({file_path, importer});
    }

    return true;
}

std::shared_ptr<Mesh> ResourceImporter::meshOfNode(int ai_mesh_idx)
{
    aiMesh *ai_mesh = m_scene->mMeshes[ai_mesh_idx];
    std::shared_ptr<Mesh> res = load_sub_mesh_data(ai_mesh);
    res->sub_mesh_idx = ai_mesh_idx;
    aiMaterial *ai_material = m_scene->mMaterials[ai_mesh->mMaterialIndex];
    res->material = load_material(ai_material);
    return res;
}

std::shared_ptr<Material> ResourceImporter::materialOfNode(int ai_mesh_idx)
{
    auto ai_mesh = m_scene->mMeshes[ai_mesh_idx];
    assert(ai_mesh->mMaterialIndex >= 0);
    aiMaterial *material = m_scene->mMaterials[ai_mesh->mMaterialIndex];
    return load_material(material);
}

std::vector<int> ResourceImporter::getSubMeshesIds() const
{
    std::vector<int> res;
    for (int i = 0; i < m_scene->mNumMeshes; i++)
    {
        res.push_back(i);
    }
    return res;
}

bool ResourceImporter::hasAnimation() const
{
    return m_scene && m_scene->mNumAnimations > 0;
}

std::vector<aiMesh *> ResourceImporter::collect_ai_meshes()
{
    std::vector<aiMesh *> res;

    std::vector<aiNode *> nodes{m_scene->mRootNode};
    while (!nodes.empty())
    {
        aiNode *node = nodes.front();
        nodes.erase(nodes.begin());
        for (int i = 0; i < node->mNumMeshes; i++)
        {
            res.push_back(m_scene->mMeshes[node->mMeshes[i]]);
        }
        for (int i = 0; i < node->mNumChildren; i++)
        {
            nodes.push_back(node->mChildren[i]);
        }
    }
    return res;
}

std::shared_ptr<Mesh> ResourceImporter::load_sub_mesh_data(aiMesh *mesh)
{
    std::vector<Vertex> vertices;
    std::vector<int> indices;

    // 处理顶点位置、法线和纹理坐标
    for (int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        Vec3 position;
        position.x = mesh->mVertices[i].x;
        position.y = mesh->mVertices[i].y;
        position.z = mesh->mVertices[i].z;
        vertex.position = position;

        if (mesh->HasNormals())
        {
            Vec3 normal;
            normal.x = mesh->mNormals[i].x;
            normal.y = mesh->mNormals[i].y;
            normal.z = mesh->mNormals[i].z;
            vertex.normal = normal;
        }

        if (mesh->mTextureCoords[0]) // 网格是否有纹理坐标？
        {
            Vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texture_uv = vec;
        }
        else
            vertex.texture_uv = Vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }

    // 处理索引
    for (int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    extractBoneWeightForVertices(vertices, mesh);

    return std::make_shared<Mesh>(vertices, indices);
}

void ResourceImporter::setVertexBoneData(Vertex &vertex, int bone_id, float weight) const
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (vertex.bone_ids[i] < 0)
        {
            vertex.bone_ids[i] = bone_id;
            vertex.bone_weights[i] = weight;
            return;
        }
    }
}

void ResourceImporter::extractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh *mesh)
{
    for (unsigned int bone_idx = 0; bone_idx < mesh->mNumBones; ++bone_idx)
    {
        aiBone *bone = mesh->mBones[bone_idx];
        std::string bone_name = bone->mName.C_Str();
        if (m_BoneInfoMap.find(bone_name) == m_BoneInfoMap.end())
        {
            BoneInfo new_bone_info;
            new_bone_info.id = m_BoneCounter++;
            new_bone_info.offset = toMat4(bone->mOffsetMatrix);
            m_BoneInfoMap[bone_name] = new_bone_info;
        }

        const int bone_id = m_BoneInfoMap[bone_name].id;
        for (unsigned int weight_idx = 0; weight_idx < bone->mNumWeights; ++weight_idx)
        {
            const int vertex_id = bone->mWeights[weight_idx].mVertexId;
            const float weight = bone->mWeights[weight_idx].mWeight;
            if (vertex_id >= 0 && vertex_id < static_cast<int>(vertices.size()))
                setVertexBoneData(vertices[vertex_id], bone_id, weight);
        }
    }
}

std::shared_ptr<Material> ResourceImporter::load_material(aiMaterial *material)
{
    std::shared_ptr<Material> res = std::make_shared<Material>();

    aiString str;
    if (material->GetTextureCount(aiTextureType_DIFFUSE))
    {
        material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
        res->diffuse_texture = std::make_shared<Texture>(TextureType::Diffuse, m_directory + '/' + std::string(str.C_Str()), false);
    }

    if (material->GetTextureCount(aiTextureType_SPECULAR))
    {
        material->GetTexture(aiTextureType_SPECULAR, 0, &str);
        res->specular_texture = std::make_shared<Texture>(TextureType::Specular, m_directory + '/' + std::string(str.C_Str()), false);
    }

    if (material->GetTextureCount(aiTextureType_NORMALS))
    {
        material->GetTexture(aiTextureType_NORMALS, 0, &str);
        res->normal_texture = std::make_shared<Texture>(TextureType::Normal, m_directory + '/' + std::string(str.C_Str()), false);
    }

    if (material->GetTextureCount(aiTextureType_HEIGHT))
    {
        material->GetTexture(aiTextureType_HEIGHT, 0, &str);
        res->height_texture = std::make_shared<Texture>(TextureType::Height, m_directory + '/' + std::string(str.C_Str()), false);
    }

    return res;
}

