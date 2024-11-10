#pragma once
#include "Mesh.h"

namespace Framework
{
	class Animation;
	class Animator;

	class AnimatedMesh :
		public Mesh
	{
	public:
		AnimatedMesh(const std::string& filePath);
		~AnimatedMesh();

		void Draw(const Camera& camera, const glm::mat4 modelMatrix, const Animator* animator) const;

		static constexpr size_t sMaxNumOfBonesPerVertex = 4;

		static constexpr int sNullBoneIndex = 0;

		void SetBoneIds(std::vector<glm::ivec4> ids);
		void SetBoneWeights(std::vector<glm::vec4> weights);

		struct BoneData
		{
			int mIndex{};
			glm::mat4 mOffset{};
		};
		inline std::unordered_map<std::string, BoneData>& GetBoneLookUp() { return mBoneLookUp; }

		// The name of animations is different across different versions of assimp. To allow portability to the Pi, it's safer to get an animation based on the index.
		inline Animation* GetAnimation(const std::string& name) { return mAnimationLookUp.at(name); }
		inline Animation* GetAnimation(const size_t index) { return mAnimations.at(index).get(); }

	private:
		void LoadFrom(const std::string& filePath, const aiScene* scene) override;

		std::unordered_map<std::string, BoneData> mBoneLookUp{};

		std::unordered_map<std::string, Animation*> mAnimationLookUp{};
		std::vector<std::unique_ptr<Animation>> mAnimations{};

		std::vector<glm::ivec4> mBoneIds{};
		std::vector<glm::vec4> mBoneWeights{};

		GLuint mBoneIdsBuffer{};
		GLuint mBoneWeightsBuffer{};
	};
}
