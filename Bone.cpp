#include "precomp.h"
#include "Bone.h"

Framework::Bone::Bone(const std::string& name, int index, const aiNodeAnim* channel) :
	mName(name),
	mIndex(index)
{
	for (uint positionIndex = 0; positionIndex < channel->mNumPositionKeys; ++positionIndex)
	{
		aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
		float timeStamp = static_cast<float>(channel->mPositionKeys[positionIndex].mTime);
		KeyFrame<glm::vec3> keyFrame{};
		keyFrame.mData = Math::ToGLM(aiPosition);
		keyFrame.mTimeStamp = timeStamp;
		mPositions.push_back(keyFrame);
	}

	for (uint rotationIndex = 0; rotationIndex < channel->mNumRotationKeys; ++rotationIndex)
	{
		aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
		float timeStamp = static_cast<float>(channel->mRotationKeys[rotationIndex].mTime);
		KeyFrame<glm::quat> keyFrame{};
		keyFrame.mData = Math::ToGLM(aiOrientation);
		keyFrame.mTimeStamp = timeStamp;
		mOrientations.push_back(keyFrame);
	}

	for (uint keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex)
	{
		aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
		float timeStamp = static_cast<float>(channel->mScalingKeys[keyIndex].mTime);
		KeyFrame<glm::vec3> keyFrame{};
		keyFrame.mData = Math::ToGLM(scale);
		keyFrame.mTimeStamp = timeStamp;
		mScales.push_back(keyFrame);
	}
}

glm::mat4 Framework::Bone::GetLocalMatrix(float atAnimationTime) const
{
	glm::mat4 translation = InterpolatePosition(atAnimationTime);
	glm::mat4 rotation = InterpolateRotation(atAnimationTime);
	glm::mat4 scale = InterpolateScaling(atAnimationTime);
	return translation * rotation * scale;
}

float Framework::Bone::GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const
{
	const float midWayLength = animationTime - lastTimeStamp;
	const float framesDiff = nextTimeStamp - lastTimeStamp;
	return midWayLength / framesDiff;
}

glm::mat4 Framework::Bone::InterpolatePosition(float animationTime) const
{
	if (mPositions.size() == 1)
	{
		return glm::translate(glm::mat4(1.0f), mPositions[0].mData);
	}

	const size_t p0Index = GetIndex(mPositions, animationTime);
	const size_t p1Index = p0Index + 1;
	const float scaleFactor = GetScaleFactor(mPositions[p0Index].mTimeStamp, mPositions[p1Index].mTimeStamp, animationTime);
	const glm::vec3 finalPosition = glm::mix(mPositions[p0Index].mData, mPositions[p1Index].mData, scaleFactor);
	return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 Framework::Bone::InterpolateRotation(float animationTime) const
{
	if (mOrientations.size() == 1)
	{
		return glm::toMat4(glm::normalize(mOrientations[0].mData));
	}

	const size_t p0Index = GetIndex(mOrientations, animationTime);
	const size_t p1Index = p0Index + 1;
	const float scaleFactor = GetScaleFactor(mOrientations[p0Index].mTimeStamp, mOrientations[p1Index].mTimeStamp, animationTime);
	const glm::quat finalRotation = glm::slerp(mOrientations[p0Index].mData, mOrientations[p1Index].mData, scaleFactor);
	return glm::toMat4(glm::normalize(finalRotation));
}

glm::mat4 Framework::Bone::InterpolateScaling(float animationTime) const
{
	if (mScales.size() == 1)
	{
		return glm::scale(glm::mat4(1.0f), mScales[0].mData);
	}

	const size_t p0Index = GetIndex(mScales, animationTime);
	const size_t p1Index = p0Index + 1;
	const float scaleFactor = GetScaleFactor(mScales[p0Index].mTimeStamp, mScales[p1Index].mTimeStamp, animationTime);
	const glm::vec3 finalScale = glm::mix(mScales[p0Index].mData, mScales[p1Index].mData, scaleFactor);
	return glm::scale(glm::mat4(1.0f), finalScale);
}
