#include "precomp.h"
#include "Animator.h"

Framework::Animator::Animator(const Animation* startingAnimation)
{
	PlayAnimation(startingAnimation);
}

void Framework::Animator::UpdateAnimation(const float deltaTime)
{
	if (mCurrentAnimation != nullptr)
	{
		float previousTime = mCurrentTime;
		mCurrentTime += mCurrentAnimation->GetTicksPerSecond() * deltaTime;
		mCurrentTime = mLoopAnimation ? fmodf(mCurrentTime, mCurrentAnimation->GetDuration()) : std::min(mCurrentTime, mCurrentAnimation->GetDuration());

		if (previousTime != mCurrentTime)
		{
			CalculateBoneTransform(mCurrentAnimation->GetRootNode(), glm::mat4(1.0f));
		}
	}
}

void Framework::Animator::PlayAnimation(const Animation* animation)
{
	mCurrentAnimation = animation;
	mCurrentTime = 0.0f;
	CalculateBoneTransform(mCurrentAnimation->GetRootNode(), glm::mat4(1.0f));
}

void Framework::Animator::CalculateBoneTransform(const AssimpNodeData& node, const glm::mat4& parentTransform)
{
	const std::optional<const Bone*> bone = node.mBone;
	const glm::mat4 nodeTransform = bone.has_value() ? bone.value()->GetLocalMatrix(mCurrentTime) : node.mTransformMatrix;
	const glm::mat4 globalTransformation = parentTransform * nodeTransform;

	if (bone.has_value())
	{
		const size_t boneIndex = bone.value()->GetBoneIndex();

		if (boneIndex >= mFinalBoneMatrices.size())
		{
			mFinalBoneMatrices.resize(boneIndex + 1);
		}

		mFinalBoneMatrices[boneIndex] = globalTransformation * bone.value()->GetOffset();
	}

	for (const AssimpNodeData& child : node.mChildren)
	{
		CalculateBoneTransform(child, globalTransformation);
	}
}