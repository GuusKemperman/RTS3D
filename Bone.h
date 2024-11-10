#pragma once


namespace Framework
{
	// Based on https://learnopengl.com/Guest-Articles/2020/Skeletal-Animation
	class Bone
	{
		template<typename DataType>
		struct KeyFrame
		{
			DataType mData{};
			float mTimeStamp{};
		};

	public:
		Bone(const std::string& name, int index, const aiNodeAnim* channel);

		glm::mat4 GetLocalMatrix(float atAnimationTime) const;

		const std::string& GetBoneName() const { return mName; }
		int GetBoneIndex() const { return mIndex; }
		const glm::mat4& GetOffset() const { assert(mOffset != nullptr); return *mOffset; }
		void SetOffset(const glm::mat4* offset) { mOffset = offset; }

	private:
		template<typename T>
		inline size_t GetIndex(const std::vector<KeyFrame<T>>& source, float animationTime) const
		{
			for (size_t index = 0; index < source.size() - 1; index++)
			{
				if (animationTime <= source[index + 1].mTimeStamp)
				{
					return index;
				}
			}
			assert(false);
			return 0;
		}

		float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const;

		glm::mat4 InterpolatePosition(float animationTime) const;
		glm::mat4 InterpolateRotation(float animationTime) const;
		glm::mat4 InterpolateScaling(float animationTime) const;

	private:
		std::vector<KeyFrame<glm::vec3>> mPositions{};
		std::vector<KeyFrame<glm::quat>> mOrientations{};
		std::vector<KeyFrame<glm::vec3>> mScales{};

		const glm::mat4* mOffset{};

		std::string mName{};
		int mIndex{};
	};
}