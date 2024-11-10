#pragma once
#include "Bone.h"


namespace Framework
{
    class AnimatedMesh;

    struct AssimpNodeData
    {
        glm::mat4 mTransformMatrix{};
        std::string mName{};
        std::vector<AssimpNodeData> mChildren{};
        std::optional<const Bone*> mBone{};
    };

    // Based on https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation
    class Animation
    {
    public:
        Animation(const aiAnimation* animation, const aiScene* scene, AnimatedMesh& mesh);

        inline float GetTicksPerSecond() const { return mTicksPerSeconds; }
        inline float GetDuration() const { return mDuration; }
        inline const AssimpNodeData& GetRootNode() const { return mRootNode; }

    private:
        const Bone* FindBone(const std::string& name) const;
        void ReadMissingBones(const aiAnimation* animation, AnimatedMesh& mesh);
        void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src);

        AnimatedMesh& mMadeForMesh;

        float mDuration{};
        float mTicksPerSeconds{};
        std::vector<Bone> mBones{};
        AssimpNodeData mRootNode{};
    };
}