#include "precomp.h"
#include "Animation.h"

#include "AnimatedMesh.h"

Framework::Animation::Animation(const aiAnimation* animation, const aiScene* scene, AnimatedMesh& mesh) :
    mMadeForMesh(mesh)
{
    mDuration = static_cast<float>(animation->mDuration);
    mTicksPerSeconds = static_cast<float>(animation->mTicksPerSecond);
    ReadMissingBones(animation, mesh);
    ReadHeirarchyData(mRootNode, scene->mRootNode);
}

const Framework::Bone* Framework::Animation::FindBone(const std::string& name) const
{
    auto iter = std::find_if(mBones.begin(), mBones.end(),
        [&](const Bone& Bone)
        {
            return Bone.GetBoneName() == name;
        }
    );
    if (iter == mBones.end()) return {};
    else return &(*iter);
}

void Framework::Animation::ReadMissingBones(const aiAnimation* animation, AnimatedMesh& mesh)
{
    uint size = animation->mNumChannels;

    std::unordered_map<std::string, AnimatedMesh::BoneData>& boneLookUp = mesh.GetBoneLookUp();

    for (uint i = 0; i < size; i++)
    {
        aiNodeAnim* channel = animation->mChannels[i];
        const std::string boneName = channel->mNodeName.C_Str();

        auto boneIt = boneLookUp.find(boneName);

        assert(boneIt != boneLookUp.end());

        mBones.emplace_back(channel->mNodeName.data, boneIt->second.mIndex, channel);
        mBones.back().SetOffset(&boneIt->second.mOffset);
    }
}

void Framework::Animation::ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
{
    assert(src);

    dest.mName = src->mName.data;
    dest.mTransformMatrix = Math::ToGLM(src->mTransformation);

    const Bone* bone = FindBone(dest.mName);
    if (bone != nullptr)
    {
        dest.mBone = bone;
    }

    for (uint i = 0; i < src->mNumChildren; i++)
    {
        AssimpNodeData newData;
        ReadHeirarchyData(newData, src->mChildren[i]);
        dest.mChildren.push_back(newData);
    }
}
