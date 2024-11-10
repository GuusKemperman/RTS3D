#pragma once

namespace Framework
{
	class BoundingBox2D
	{
	public:
		BoundingBox2D() = default;
		BoundingBox2D(const glm::vec2& topLeft, const glm::vec2& size)
		{
			mSides.mLeft = topLeft.x, mSides.mTop = topLeft.y, mSides.mWidth = size.x, mSides.mHeight = size.y;
		}
		BoundingBox2D(const glm::vec4& a_values)
		{
			mData = a_values;
		}

		inline bool Contains(const glm::vec2 point) const
		{
			return point.x >= mSides.mLeft
				&& point.x <= GetRight()
				&& point.y >= mSides.mTop
				&& point.y <= GetBottom();
		}

		inline bool Contains(const BoundingBox2D& box) const
		{
			return mSides.mLeft <= box.mSides.mLeft
				&& mSides.mTop <= box.mSides.mTop
				&& box.GetRight() <= GetRight()
				&& box.GetBottom() <= GetBottom();
		}

		inline bool Intersects(const BoundingBox2D& box) const
		{
			return mSides.mLeft < box.GetRight()
				&& GetRight() > box.mSides.mLeft
				&& mSides.mTop < box.GetBottom()
				&& GetBottom() > box.mSides.mTop;
		}

		inline float GetLeft() const { return mSides.mLeft; }
		inline float GetTop() const { return mSides.mTop; }
		inline float GetRight() const { return mSides.mLeft + mSides.mWidth; }
		inline float GetBottom() const { return mSides.mTop + mSides.mHeight; }
		inline glm::vec2 GetTopLeft() const { return glm::vec2(mSides.mLeft, mSides.mTop); }
		inline glm::vec2 GetBottomRight() const { return glm::vec2(GetRight(), GetBottom()); }
		inline glm::vec2 GetTopRight() const { return glm::vec2{ GetRight(), mSides.mTop}; }
		inline glm::vec2 GetBottomLeft() const { return glm::vec2{ mSides.mLeft, GetBottom() }; }

		inline glm::vec2 GetSize() const { return glm::vec2(mSides.mWidth, mSides.mHeight); }

		inline glm::vec2 GetCentre() const { return glm::vec2(mSides.mLeft + mSides.mWidth / 2.0f, mSides.mTop + mSides.mHeight / 2.0f); }
		inline glm::vec4& GetBoxValues() { return mData; }

		struct Sides { float mLeft, mTop, mWidth, mHeight; };
		union { Sides mSides{}; glm::vec4 mData; };
	};
}