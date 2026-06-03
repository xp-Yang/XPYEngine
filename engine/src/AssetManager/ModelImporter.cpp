#include "AssetManager/ModelImporter.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "AssetManager/Mesh.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

std::unordered_map<std::string, Assimp::Importer *> ModelImporter::m_importers;
std::unordered_map<std::string, ModelGeometryCacheEntry> ModelImporter::m_geometry_cache;

static std::shared_ptr<Texture> textureOfUnknownType(aiMaterial* material, TextureType engine_type, const std::string& directory, bool gamma,
    std::initializer_list<const char*> keywords, std::initializer_list<const char*> rejected_keywords = {})
{
    if (!material)
        return nullptr;

    auto containsAny = [](std::string text, std::initializer_list<const char*> needles)
    {
        std::transform(text.begin(), text.end(), text.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        for (const char* needle : needles)
        {
            if (text.find(needle) != std::string::npos)
                return true;
        }
        return false;
    };

    const unsigned int count = material->GetTextureCount(aiTextureType_UNKNOWN);
    for (unsigned int i = 0; i < count; ++i)
    {
        aiString aiPath;
        if (material->GetTexture(aiTextureType_UNKNOWN, i, &aiPath) != AI_SUCCESS)
            continue;

        std::string path = aiPath.C_Str();
        if (!containsAny(path, keywords) || containsAny(path, rejected_keywords))
            continue;

        const std::string resolved_path = directory + '/' + path;
        return resolved_path.empty() ? nullptr : std::make_shared<Texture>(engine_type, resolved_path, gamma);
    }
    return nullptr;
}

static std::shared_ptr<Texture> textureOfType(aiMaterial* material, aiTextureType ai_type, TextureType engine_type, const std::string& directory, bool gamma)
{
    if (!material || material->GetTextureCount(ai_type) == 0)
        return nullptr;

    aiString aiPath;
    if (material->GetTexture(ai_type, 0, &aiPath) != AI_SUCCESS)
        return nullptr;

    const std::string resolved_path = directory + '/' + aiPath.C_Str();
    return resolved_path.empty() ? nullptr : std::make_shared<Texture>(engine_type, resolved_path, gamma);
};

static Mat4 toMat4(const aiMatrix4x4 &mat)
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

ModelImporter::~ModelImporter()
{
}

bool ModelImporter::load(const std::string &file_path)
{
    m_obj_filepath = PathService::normalize(file_path);
    m_directory = PathService::getDirectory(m_obj_filepath);
    m_BoneInfoMap.clear();
    m_BoneCounter = 0;

    if (auto cache_it = ModelImporter::m_geometry_cache.find(m_obj_filepath); cache_it != ModelImporter::m_geometry_cache.end())
    {
        m_BoneInfoMap = cache_it->second.bone_info_map;
        m_BoneCounter = cache_it->second.bone_count;
    }

    if (ModelImporter::m_importers.find(m_obj_filepath) != ModelImporter::m_importers.end())
    {
        m_scene = ModelImporter::m_importers.at(m_obj_filepath)->GetScene();
    }
    else
    {
        auto importer = new Assimp::Importer();
        m_scene = importer->ReadFile(m_obj_filepath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
        if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode)
        {
            auto error_str = importer->GetErrorString();
            delete importer;
            return false;
        }
        ModelImporter::m_importers.insert({m_obj_filepath, importer});
    }

    return true;
}

std::shared_ptr<Mesh> ModelImporter::meshOfNode(int ai_mesh_idx)
{
    aiMesh *ai_mesh = m_scene->mMeshes[ai_mesh_idx];
    auto& cache = ModelImporter::m_geometry_cache[m_obj_filepath];
    m_BoneInfoMap = cache.bone_info_map;
    m_BoneCounter = cache.bone_count;

    std::shared_ptr<MeshGeometry> geometry;
    auto geometry_it = cache.geometries.find(ai_mesh_idx);
    if (geometry_it != cache.geometries.end())
    {
        geometry = geometry_it->second;
    }
    else
    {
        geometry = load_sub_mesh_geometry(ai_mesh);
        cache.geometries[ai_mesh_idx] = geometry;
        cache.bone_info_map = m_BoneInfoMap;
        cache.bone_count = m_BoneCounter;
    }

    std::shared_ptr<Mesh> res = std::make_shared<Mesh>(geometry);
    res->sub_mesh_idx = ai_mesh_idx;
    aiMaterial *ai_material = m_scene->mMaterials[ai_mesh->mMaterialIndex];
    res->material = load_material(ai_material);
    return res;
}

std::shared_ptr<Material> ModelImporter::materialOfNode(int ai_mesh_idx)
{
    auto ai_mesh = m_scene->mMeshes[ai_mesh_idx];
    assert(ai_mesh->mMaterialIndex >= 0);
    aiMaterial *material = m_scene->mMaterials[ai_mesh->mMaterialIndex];
    return load_material(material);
}

// TODO 可以这样吗？
std::vector<int> ModelImporter::getSubMeshesIds() const
{
    std::vector<int> res;
    for (int i = 0; i < m_scene->mNumMeshes; i++)
    {
        res.push_back(i);
    }
    return res;
}

bool ModelImporter::hasAnimation() const
{
    return m_scene && m_scene->mNumAnimations > 0;
}

std::vector<aiMesh *> ModelImporter::collect_ai_meshes()
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

std::shared_ptr<MeshGeometry> ModelImporter::load_sub_mesh_geometry(aiMesh *mesh)
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

        if (mesh->HasTangentsAndBitangents())
        {
            Vec3 t(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            Vec3 b(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            // 手性：镜像 UV 处 assimp 的 bitangent 与 cross(N,T) 反向，记 w=-1 供着色器还原副切线。
            float handedness = (glm::dot(glm::cross(vertex.normal, t), b) < 0.0f) ? -1.0f : 1.0f;
            vertex.tangent = Vec4(t, handedness);
        }

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

    return std::make_shared<MeshGeometry>(vertices, indices);
}

void ModelImporter::setVertexBoneData(Vertex &vertex, int bone_id, float weight) const
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

void ModelImporter::extractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh *mesh)
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

std::shared_ptr<Material> ModelImporter::load_material(aiMaterial *material)
{
    std::shared_ptr<Material> res = Material::create_complete_default_material();

    auto materialLooksLikePBR = [](aiMaterial* material, const std::string& filepath)
    {
        const std::string ext = PathService::getFileSuffix(filepath);
        if (ext == "gltf" || ext == "glb")
            return true;

        int shading_model = aiShadingMode_NoShading;
        if (material && material->Get(AI_MATKEY_SHADING_MODEL, shading_model) == AI_SUCCESS)
            return shading_model == aiShadingMode_CookTorrance;

        return false;
    };

    if (materialLooksLikePBR(material, m_obj_filepath))
    {
        res->base_color_factor = res->diffuse_factor;
        auto albedo_texture = textureOfUnknownType(material, TextureType::Albedo, m_directory, true,
            { "basecolor", "base_color", "albedo" });
        if (!albedo_texture)
            albedo_texture = textureOfType(material, aiTextureType_DIFFUSE, TextureType::Albedo, m_directory, true);
        if (albedo_texture)
            res->albedo_texture = albedo_texture;

        auto metallic_texture = textureOfUnknownType(material, TextureType::Metallic, m_directory, false,
            { "metallic", "metalness" }, { "roughness" });
        if (metallic_texture)
            res->metallic_texture = metallic_texture;
        auto roughness_texture = textureOfUnknownType(material, TextureType::Roughness, m_directory, false,
            { "roughness" }, { "metallic", "metalness" });
        if (roughness_texture)
            res->roughness_texture = roughness_texture;
        auto ao_texture = textureOfType(material, aiTextureType_LIGHTMAP, TextureType::AO, m_directory, false);
        if (!ao_texture)
            ao_texture = textureOfUnknownType(material, TextureType::AO, m_directory, false,
                { "ao", "occlusion", "ambientocclusion", "ambient_occlusion" });
        if (ao_texture)
            res->ao_texture = ao_texture;

        res->metallic_factor = metallic_texture ? 1.0f : 0.0f;
        res->roughness_factor = roughness_texture ? 1.0f : 0.8f;
        res->ao_factor = 1.0f;

        res->fillBlinnPhongFromPBR();
    }
    else
    {
        aiColor3D diffuse_color(1.0f, 1.0f, 1.0f);
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color) == AI_SUCCESS)
            res->diffuse_factor = Vec3(diffuse_color.r, diffuse_color.g, diffuse_color.b);

        aiColor3D specular_color(1.0f, 1.0f, 1.0f);
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular_color) == AI_SUCCESS)
            res->specular_factor = Vec3(specular_color.r, specular_color.g, specular_color.b);

        float shininess = res->shininess;
        if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS)
            res->shininess = std::max(1.0f, std::min(shininess, 1024.0f));

        if (auto texture = textureOfType(material, aiTextureType_DIFFUSE, TextureType::Diffuse, m_directory, true))
            res->diffuse_texture = texture;
        if (auto texture = textureOfType(material, aiTextureType_SPECULAR, TextureType::Specular, m_directory, false))
            res->specular_texture = texture;
        if (auto texture = textureOfType(material, aiTextureType_NORMALS, TextureType::Normal, m_directory, false))
            res->normal_texture = texture;
        else if (auto texture = textureOfType(material, aiTextureType_HEIGHT, TextureType::Normal, m_directory, false))
            res->normal_texture = texture;
        if (auto texture = textureOfType(material, aiTextureType_DISPLACEMENT, TextureType::Height, m_directory, false))
            res->height_texture = texture;
        else if (auto texture = textureOfType(material, aiTextureType_HEIGHT, TextureType::Height, m_directory, false))
            res->height_texture = texture;

        res->fillPBRFromBlinnPhong();
    }

    return res;
}

