#include "AssetManager/ModelImporter.hpp"
#include <assimp/Importer.hpp>
#include <assimp/GltfMaterial.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "AssetManager/Mesh.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>

std::unordered_map<std::string, Assimp::Importer *> ModelImporter::m_importers;

static std::string lowerString(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

static bool containsAny(std::string text, std::initializer_list<const char*> needles)
{
    text = lowerString(text);
    for (const char* needle : needles)
    {
        if (text.find(needle) != std::string::npos)
            return true;
    }
    return false;
}

static bool isGltfModelFile(const std::string& filepath)
{
    const std::string ext = lowerString(PathService::getFileSuffix(filepath));
    return ext == "gltf" || ext == "glb";
}

static std::shared_ptr<Texture> textureFromAssimpPath(const aiScene* scene, TextureType engine_type, const std::string& directory,
    const aiString& aiPath, bool gamma)
{
    const std::string path = aiPath.C_Str();
    if (path.empty())
        return nullptr;

    if (scene && scene->GetEmbeddedTexture(path.c_str()))
    {
        Logger::warn("ModelImporter: embedded texture '{}' is not loaded yet.", path);
        return nullptr;
    }

    if (path.rfind("data:", 0) == 0)
    {
        Logger::warn("ModelImporter: data URI texture is not loaded yet.");
        return nullptr;
    }

    const std::string resolved_path = PathService::join(directory, path);
    std::error_code ec;
    if (!std::filesystem::exists(resolved_path, ec))
    {
        Logger::warn("ModelImporter: texture file not found: {}", resolved_path);
        return nullptr;
    }

    return std::make_shared<Texture>(engine_type, resolved_path, gamma);
}

static std::shared_ptr<Texture> textureOfUnknownType(const aiScene* scene, aiMaterial* material, TextureType engine_type,
    const std::string& directory, bool gamma, std::initializer_list<const char*> keywords,
    std::initializer_list<const char*> rejected_keywords = {})
{
    if (!material)
        return nullptr;

    const unsigned int count = material->GetTextureCount(aiTextureType_UNKNOWN);
    for (unsigned int i = 0; i < count; ++i)
    {
        aiString aiPath;
        if (material->GetTexture(aiTextureType_UNKNOWN, i, &aiPath) != AI_SUCCESS)
            continue;

        std::string path = aiPath.C_Str();
        if (!containsAny(path, keywords) || containsAny(path, rejected_keywords))
            continue;

        return textureFromAssimpPath(scene, engine_type, directory, aiPath, gamma);
    }
    return nullptr;
}

static std::shared_ptr<Texture> textureOfType(const aiScene* scene, aiMaterial* material, aiTextureType ai_type, TextureType engine_type,
    const std::string& directory, bool gamma)
{
    if (!material || material->GetTextureCount(ai_type) == 0)
        return nullptr;

    aiString aiPath;
    if (material->GetTexture(ai_type, 0, &aiPath) != AI_SUCCESS)
        return nullptr;

    return textureFromAssimpPath(scene, engine_type, directory, aiPath, gamma);
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

    if (ModelImporter::m_importers.find(m_obj_filepath) != ModelImporter::m_importers.end())
    {
        m_scene = ModelImporter::m_importers.at(m_obj_filepath)->GetScene();
    }
    else
    {
        auto importer = new Assimp::Importer();
        constexpr unsigned int import_flags =
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_ImproveCacheLocality;
        m_scene = importer->ReadFile(m_obj_filepath, import_flags);
        if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode)
        {
            auto error_str = importer->GetErrorString();
            Logger::error("Assimp failed to load model: {}, error: {}", m_obj_filepath, error_str);
            delete importer;
            return false;
        }
        ModelImporter::m_importers.insert({m_obj_filepath, importer});
    }

    m_meshes = collectMeshes();

    return true;
}

bool ModelImporter::hasAnimation() const
{
    return m_scene && m_scene->mNumAnimations > 0;
}

std::vector<std::shared_ptr<Mesh>> ModelImporter::collectMeshes()
{
    std::vector<aiMesh *> ai_meshes;

    std::vector<aiNode *> nodes{m_scene->mRootNode};
    while (!nodes.empty())
    {
        aiNode *node = nodes.front();
        nodes.erase(nodes.begin());
        for (int i = 0; i < node->mNumMeshes; i++)
        {
            ai_meshes.push_back(m_scene->mMeshes[node->mMeshes[i]]);
        }
        for (int i = 0; i < node->mNumChildren; i++)
        {
            nodes.push_back(node->mChildren[i]);
        }
    }

    std::vector<std::shared_ptr<Mesh>> meshes;
    for (int i = 0; i < ai_meshes.size(); i++)
    {
        aiMesh *ai_mesh = ai_meshes[i];
        std::shared_ptr<MeshGeometry> geometry = loadMeshGeometry(ai_mesh);
        aiMaterial *ai_material = m_scene->mMaterials[ai_mesh->mMaterialIndex];
        std::shared_ptr<Material> material = loadMaterial(ai_material);
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(geometry, material);
        mesh->sub_mesh_idx = i;
        meshes.push_back(mesh);
    }

    return meshes;
}

std::shared_ptr<MeshGeometry> ModelImporter::loadMeshGeometry(aiMesh *mesh)
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

std::shared_ptr<Material> ModelImporter::loadMaterial(aiMaterial *material)
{
    std::shared_ptr<Material> res = Material::create_complete_default_material();
    if (!material)
        return res;

    auto materialLooksLikePBR = [](aiMaterial* material, const std::string& filepath)
    {
        if (isGltfModelFile(filepath))
            return true;

        float factor = 0.0f;
        aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
        if (material && material->Get(AI_MATKEY_BASE_COLOR, base_color) == AI_SUCCESS)
            return true;
        if (material && material->Get(AI_MATKEY_METALLIC_FACTOR, factor) == AI_SUCCESS)
            return true;
        if (material && material->Get(AI_MATKEY_ROUGHNESS_FACTOR, factor) == AI_SUCCESS)
            return true;
        if (material && (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0 ||
            material->GetTextureCount(aiTextureType_METALNESS) > 0 ||
            material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0))
            return true;

        int shading_model = aiShadingMode_NoShading;
        if (material && material->Get(AI_MATKEY_SHADING_MODEL, shading_model) == AI_SUCCESS)
            return shading_model == aiShadingMode_CookTorrance;

        return false;
    };

    if (materialLooksLikePBR(material, m_obj_filepath))
    {
        aiColor4D base_color(1.0f, 1.0f, 1.0f, 1.0f);
        if (material->Get(AI_MATKEY_BASE_COLOR, base_color) == AI_SUCCESS)
        {
            res->base_color_factor = Vec3(base_color.r, base_color.g, base_color.b);
            res->diffuse_factor = res->base_color_factor;
            res->alpha = base_color.a;
        }
        else
        {
            aiColor3D diffuse_color(1.0f, 1.0f, 1.0f);
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_color) == AI_SUCCESS)
            {
                res->base_color_factor = Vec3(diffuse_color.r, diffuse_color.g, diffuse_color.b);
                res->diffuse_factor = res->base_color_factor;
            }
        }

        float opacity = res->alpha;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
            res->alpha = std::clamp(opacity, 0.0f, 1.0f);

        res->metallic_factor = isGltfModelFile(m_obj_filepath) ? 1.0f : 0.0f;
        res->roughness_factor = isGltfModelFile(m_obj_filepath) ? 1.0f : 0.8f;

        float metallic_factor = res->metallic_factor;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic_factor) == AI_SUCCESS)
            res->metallic_factor = std::clamp(metallic_factor, 0.0f, 1.0f);

        float roughness_factor = res->roughness_factor;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_factor) == AI_SUCCESS)
            res->roughness_factor = std::clamp(roughness_factor, 0.04f, 1.0f);

        auto albedo_texture = textureOfType(m_scene, material, aiTextureType_BASE_COLOR, TextureType::Albedo, m_directory, true);
        if (!albedo_texture)
            albedo_texture = textureOfType(m_scene, material, aiTextureType_DIFFUSE, TextureType::Albedo, m_directory, true);
        if (!albedo_texture)
            albedo_texture = textureOfUnknownType(m_scene, material, TextureType::Albedo, m_directory, true,
                { "basecolor", "base_color", "albedo", "diffuse" });
        if (albedo_texture)
            res->albedo_texture = albedo_texture;

        auto metallic_texture = textureOfType(m_scene, material, aiTextureType_METALNESS, TextureType::Metallic, m_directory, false);
        if (!metallic_texture)
            metallic_texture = textureOfType(m_scene, material, aiTextureType_GLTF_METALLIC_ROUGHNESS, TextureType::Metallic, m_directory, false);
        if (!metallic_texture)
            metallic_texture = textureOfUnknownType(m_scene, material, TextureType::Metallic, m_directory, false,
                { "metallic", "metalness" }, { "roughness" });
        if (metallic_texture)
            res->metallic_texture = metallic_texture;

        auto roughness_texture = textureOfType(m_scene, material, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::Roughness, m_directory, false);
        if (!roughness_texture)
            roughness_texture = textureOfType(m_scene, material, aiTextureType_GLTF_METALLIC_ROUGHNESS, TextureType::Roughness, m_directory, false);
        if (!roughness_texture)
            roughness_texture = textureOfUnknownType(m_scene, material, TextureType::Roughness, m_directory, false,
                { "roughness" }, { "metallic", "metalness" });
        if (roughness_texture)
            res->roughness_texture = roughness_texture;

        auto ao_texture = textureOfType(m_scene, material, aiTextureType_AMBIENT_OCCLUSION, TextureType::AO, m_directory, false);
        if (!ao_texture)
            ao_texture = textureOfType(m_scene, material, aiTextureType_LIGHTMAP, TextureType::AO, m_directory, false);
        if (!ao_texture)
            ao_texture = textureOfUnknownType(m_scene, material, TextureType::AO, m_directory, false,
                { "ao", "occlusion", "ambientocclusion", "ambient_occlusion" });
        if (ao_texture)
            res->ao_texture = ao_texture;

        if (auto texture = textureOfType(m_scene, material, aiTextureType_NORMALS, TextureType::Normal, m_directory, false))
            res->normal_texture = texture;
        else if (auto texture = textureOfType(m_scene, material, aiTextureType_HEIGHT, TextureType::Normal, m_directory, false))
            res->normal_texture = texture;

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

        float opacity = res->alpha;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
            res->alpha = std::clamp(opacity, 0.0f, 1.0f);

        if (auto texture = textureOfType(m_scene, material, aiTextureType_DIFFUSE, TextureType::Diffuse, m_directory, true))
            res->diffuse_texture = texture;
        if (auto texture = textureOfType(m_scene, material, aiTextureType_SPECULAR, TextureType::Specular, m_directory, false))
            res->specular_texture = texture;
        if (auto texture = textureOfType(m_scene, material, aiTextureType_NORMALS, TextureType::Normal, m_directory, false))
            res->normal_texture = texture;
        else if (auto texture = textureOfType(m_scene, material, aiTextureType_HEIGHT, TextureType::Normal, m_directory, false))
            res->normal_texture = texture;
        if (auto texture = textureOfType(m_scene, material, aiTextureType_DISPLACEMENT, TextureType::Height, m_directory, false))
            res->height_texture = texture;
        else if (auto texture = textureOfType(m_scene, material, aiTextureType_HEIGHT, TextureType::Height, m_directory, false))
            res->height_texture = texture;

        res->fillPBRFromBlinnPhong();
    }

    return res;
}

