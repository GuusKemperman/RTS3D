#pragma once
#include "Animation.h"
#include "Bone.h"

namespace Framework
{
	// Based on https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation
	class Animator
	{
	public:
		Animator(const Animation* startingAnimation);

		void UpdateAnimation(const float deltaTime);
		void PlayAnimation(const Animation* animation);

		inline const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return mFinalBoneMatrices;	}
		inline const Animation* GetAnimation() const { return mCurrentAnimation; }

		float mCurrentTime{};
		bool mLoopAnimation = true;

	private:
		void CalculateBoneTransform(const AssimpNodeData& node, const glm::mat4& parentTransform);

		std::vector<glm::mat4> mFinalBoneMatrices{};
		const Animation* mCurrentAnimation{};
	};
}