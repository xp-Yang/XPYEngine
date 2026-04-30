#ifndef AnimationSystem_hpp
#define AnimationSystem_hpp

#include "Base/Math/Math.hpp"
#include "Animation.hpp"
#include "Logical/Framework/World/Scene.hpp"
#include <unordered_map>
#include <unordered_set>

class AnimationSystem
{
public:
    AnimationSystem() = default;

    void onUpdate(std::shared_ptr<Scene> scene, float delta_time = 1.0f / 60.0f)
    {
        if (!scene)
            return;

        std::unordered_set<int> alive_object_ids;
        for (const auto &object : scene->getObjects())
        {
            auto *animation_component = object->getComponent<AnimationComponent>();
            if (!animation_component || !animation_component->clip)
                continue;

            const int object_id = object->ID().id;
            alive_object_ids.insert(object_id);

            auto &state = m_object_states[object_id];
            if (!state.clip || state.clip.get() != animation_component->clip.get())
            {
                state.clip = animation_component->clip;
                state.current_time = 0.0f;
                state.final_bone_matrices.assign(MAX_BONE_PALETTE_SIZE, Mat4(1.0f));
            }
            state.playing = animation_component->playing;
            state.loop = animation_component->loop;
            state.speed = animation_component->speed;

            if (!state.playing || !state.clip || state.clip->GetDuration() <= 0.0f)
                continue;

            state.current_time += state.clip->GetTicksPerSecond() * delta_time * state.speed;
            if (state.loop)
                state.current_time = fmod(state.current_time, state.clip->GetDuration());
            else if (state.current_time > state.clip->GetDuration())
                state.current_time = state.clip->GetDuration();
            CalculateBoneTransform(state, &state.clip->GetRootNode(), Mat4(1.0f));
        }

        for (auto it = m_object_states.begin(); it != m_object_states.end();)
        {
            if (alive_object_ids.find(it->first) == alive_object_ids.end())
                it = m_object_states.erase(it);
            else
                ++it;
        }
    }

    const std::vector<Mat4> &GetFinalBoneMatrices(int object_id) const
    {
        auto it = m_object_states.find(object_id);
        if (it == m_object_states.end())
            return m_empty_bone_matrices;
        return it->second.final_bone_matrices;
    }

    bool HasAnimation(int object_id) const
    {
        auto it = m_object_states.find(object_id);
        return it != m_object_states.end() && it->second.clip != nullptr;
    }

private:
    struct ObjectAnimationState
    {
        std::shared_ptr<Animation> clip{ nullptr };
        std::vector<Mat4> final_bone_matrices = std::vector<Mat4>(MAX_BONE_PALETTE_SIZE, Mat4(1.0f));
        float current_time{ 0.0f };
        float speed{ 1.0f };
        bool loop{ true };
        bool playing{ true };
    };

    void CalculateBoneTransform(ObjectAnimationState &state, const AssimpNodeData *node, Mat4 parentTransform)
    {
        std::string nodeName = node->name;
        Mat4 nodeTransform = node->transformation;

        Bone *bone = state.clip->FindBone(nodeName);

        if (bone)
        {
            bone->onUpdate(state.current_time);
            nodeTransform = bone->GetLocalTransform();
        }

        Mat4 globalTransformation = parentTransform * nodeTransform;

        const auto &boneInfoMap = state.clip->GetBoneIDMap();
        if (boneInfoMap.find(nodeName) != boneInfoMap.end())
        {
            int index = boneInfoMap.at(nodeName).id;
            if (index >= 0 && index < MAX_BONE_PALETTE_SIZE)
            {
                Mat4 offset = boneInfoMap.at(nodeName).offset;
                state.final_bone_matrices[index] = globalTransformation * offset;
            }
        }

        for (int i = 0; i < node->childrenCount; i++)
            CalculateBoneTransform(state, &node->children[i], globalTransformation);
    }

    std::unordered_map<int, ObjectAnimationState> m_object_states;
    std::vector<Mat4> m_empty_bone_matrices = std::vector<Mat4>(MAX_BONE_PALETTE_SIZE, Mat4(1.0f));
};

#endif // !AnimationSystem_hpp
