#include "precomp.h"
#include "Frustum.h"

Framework::FrustumShape::FrustumShape(const float width, const float height, const float fov, const float zNear, const float zFar)
{
	const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	
	const glm::mat4 inverseProjection = glm::inverse(glm::perspective(fov, aspectRatio, zNear, zFar));

	const glm::vec3 pointsClipSpace[8]
	{
		{ -1.0f, -1.0f, 0.0f },
		{ -1.0f,  1.0f, 0.0f },
		{  1.0f, -1.0f, 0.0f },
		{  1.0f,  1.0f, 0.0f },

		{ -1.0f, -1.0f, 1.0f },
		{ -1.0f,  1.0f, 1.0f },
		{  1.0f, -1.0f, 1.0f },
		{  1.0f,  1.0f, 1.0f }
	};

	for (int i = 0; i < 8; i++)
	{
		const glm::vec3 clipSpace = pointsClipSpace[i];
		const glm::vec4 worldSpace4 = inverseProjection * glm::vec4{ clipSpace, 1.0f };
		const glm::vec3 pointWorld = -(worldSpace4 * (1.0f / worldSpace4.w));

		// Only recalulate bounding box after adding the last point
		addPoint(Math::ToBullet(pointWorld), i == 7);
	}
}