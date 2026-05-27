#ifndef Animation_hpp
#define Animation_hpp

#include "Base/Math/Math.hpp"
#include "Bone.hpp"
#include "AssetManager/ModelImporter.hpp"

struct AssimpNodeData
{
    Mat4 transformation;
    std::string name;
    int childrenCount;
    std::vector<AssimpNodeData> children;
};

class Animation
{
public:
    Animation() = default;

    Animation(const std::string& animationPath, ModelImporter* model)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
        assert(scene && scene->mRootNode && scene->mNumAnimations > 0);
        auto animation = scene->mAnimations[0];
        m_Duration = animation->mDuration;
        m_TicksPerSecond = animation->mTicksPerSecond == 0 ? 25.0f : animation->mTicksPerSecond;
        ReadHeirarchyData(m_RootNode, scene->mRootNode);
        ReadMissingBones(animation, *model);
    }

    ~Animation()
    {
    }

    Bone* FindBone(const std::string& name)
    {
        auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
            [&](const Bone& Bone)
            {
                return Bone.GetBoneName() == name;
            }
        );
        if (iter == m_Bones.end()) return nullptr;
        else return &(*iter);
    }


    inline float GetTicksPerSecond() { return m_TicksPerSecond; }

    inline float GetDuration() { return m_Duration; }

    inline const AssimpNodeData& GetRootNode() { return m_RootNode; }

    inline const std::map<std::string, BoneInfo>& GetBoneIDMap()
    {
        return m_BoneInfoMap;
    }

private:
    void ReadMissingBones(const aiAnimation* animation, ModelImporter& model)
    {
        int size = animation->mNumChannels;

        auto boneInfoMap = model.getBoneInfoMap();//getting m_BoneInfoMap from Model class
        int boneCount = model.getBoneCount(); //getting the m_BoneCounter from Model class

        //reading channels(bones engaged in an animation and their keyframes)
        for (int i = 0; i < size; i++)
        {
            auto channel = animation->mChannels[i];
            std::string boneName = channel->mNodeName.data;

            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                BoneInfo bone_info;
                bone_info.id = boneCount;
                bone_info.offset = Mat4(1.0f);
                boneInfoMap[boneName] = bone_info;
                boneCount++;
            }
            m_Bones.push_back(Bone(channel->mNodeName.data,
                boneInfoMap[channel->mNodeName.data].id, channel));
        }

        m_BoneInfoMap = boneInfoMap;
    }

    void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
    {
        assert(src);

        dest.name = src->mName.data;
        dest.transformation[0][0] = src->mTransformation.a1;
        dest.transformation[1][0] = src->mTransformation.a2;
        dest.transformation[2][0] = src->mTransformation.a3;
        dest.transformation[3][0] = src->mTransformation.a4;
        dest.transformation[0][1] = src->mTransformation.b1;
        dest.transformation[1][1] = src->mTransformation.b2;
        dest.transformation[2][1] = src->mTransformation.b3;
        dest.transformation[3][1] = src->mTransformation.b4;
        dest.transformation[0][2] = src->mTransformation.c1;
        dest.transformation[1][2] = src->mTransformation.c2;
        dest.transformation[2][2] = src->mTransformation.c3;
        dest.transformation[3][2] = src->mTransformation.c4;
        dest.transformation[0][3] = src->mTransformation.d1;
        dest.transformation[1][3] = src->mTransformation.d2;
        dest.transformation[2][3] = src->mTransformation.d3;
        dest.transformation[3][3] = src->mTransformation.d4;
        dest.childrenCount = src->mNumChildren;

        for (int i = 0; i < src->mNumChildren; i++)
        {
            AssimpNodeData newData;
            ReadHeirarchyData(newData, src->mChildren[i]);
            dest.children.push_back(newData);
        }
    }

    float m_Duration;
    float m_TicksPerSecond;
    std::vector<Bone> m_Bones;
    AssimpNodeData m_RootNode;
    std::map < std::string, BoneInfo > m_BoneInfoMap;
};

#endif // !Animation_hpp
