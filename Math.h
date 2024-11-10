#pragma once

constexpr float PI = 3.1415927358979323848f;
constexpr float INVPI = 0.31830988618379067153777f;
constexpr float INV2PI = 0.15915494309189533576888f;
constexpr float TWOPI = 6.28318530717958647692528f;
constexpr float SQRT_PI_INV = 0.56418958355f;
constexpr float LARGE_FLOAT = 1e34f;

#define DEG2RAD(x) (x*PI)/180
#define RAD2DEG(x) x*(180/PI)

namespace Framework
{
	class Math
	{
	public:
		template <class T> 
		static inline void Swap(T& x, T& y) { T t; t = x, x = y, y = t; }

		template <typename T>
		static inline constexpr T sqr(T v) { return v * v; }

		template <typename T>
		static inline constexpr T lerp(T min, T max, float t) { return min + t * (max - min); }

		template <typename T>
		static inline constexpr float lerpInv(T min, T max, T value) { return (value - min) / (max - min); }

		template <typename T>
		static inline constexpr bool IsPowerOfTwo(T x) { return (x != 0) && ((x & (x - 1)) == 0); }

		// In degrees
		static inline float Vec2ToAngle(glm::vec2 vec)
		{
			return (std::atan2(vec.y, vec.x) / TWOPI) * 360.0f + 90.0f;
		}
		// In degrees
		static inline glm::vec2 AngleToVec2(float angle)
		{
			const float radian = (angle - 90.0f) * (1.0f / 360.0f) * TWOPI;
			return { std::cos(radian), std::sin(radian) };
		}

		// Conversions
		static inline glm::vec3 ToGLM(const btVector3& v) { return { v.m_floats[0], v.m_floats[1], v.m_floats[2] }; }
		static inline glm::quat ToGLM(const btQuaternion& q) { return { q.w(), q.x(), q.y(), q.z() }; }

		static inline btVector3 ToBullet(const glm::vec3& v) { return { v.x, v.y, v.z }; }

		static inline ImVec2 ToIMGui(const glm::vec2& v) { return { v.x, v.y }; }
		static inline ImVec4 ToIMGui(const glm::vec4& v) { return { v.x, v.y, v.z, v.w }; }

		static inline glm::mat4 ToGLM(const aiMatrix4x4& from)
		{
			glm::mat4 to{};
			to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
			to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
			to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
			to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
			return to;
		}

		static inline glm::vec3 ToGLM(const aiVector3D& v) { return { v.x, v.y, v.z }; }
		static inline glm::quat ToGLM(const aiQuaternion& q) { return { q.w, q.x, q.y, q.z }; }

		// https://www.gamedeveloper.com/business/how-to-work-with-bezier-curve-in-games-with-unity For more info
		template<typename T>
		static inline constexpr T BezierCurve(const T& p0, const T& p1, const T& p2, const float t)
		{
			//B(t) = (1 - t)2P0 + 2(1 - t)tP1 + t2P2, 0 < t < 1
			return (1.0f - t) * 2.0f * p0 + 2.0f * (1.0f - t) * t * p1 + t * 2.0f * p2;
		}

		inline static glm::vec2 PixelToOpenGLPosition(const glm::ivec2& pixelPosition)
		{
			glm::vec2 openGLPos = { pixelPosition.x == 0 ? -1.0f : static_cast<float>(pixelPosition.x % static_cast<int>(sHalfScreenWidth)),
				pixelPosition.y == 0 ? -1.0f : static_cast<float>(pixelPosition.y % static_cast<int>(sHalfScreenHeight)) };

			if (pixelPosition.x < static_cast<int>(sHalfScreenWidth))
			{
				openGLPos.x = openGLPos.x - static_cast<int>(sHalfScreenWidth);
			}

			if (pixelPosition.y < static_cast<int>(sHalfScreenHeight))
			{
				openGLPos.y = openGLPos.y - static_cast<int>(sHalfScreenHeight);
			}

			if (openGLPos.x != 0.0f)
			{
				openGLPos.x /= static_cast<float>(sHalfScreenWidth);
			}

			if (openGLPos.y != 0.0f)
			{
				openGLPos.y /= -static_cast<float>(sHalfScreenHeight);
			}

			return openGLPos;
		}
	};
}