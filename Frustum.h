#pragma once

namespace Framework
{
	class FrustumShape :
		public btConvexHullShape
	{
	public:
		FrustumShape() = default;
		FrustumShape(const float width, const float height, const float fov, const float zNear, const float zFar);
	};
}