#pragma once

namespace Framework
{
	class Camera;
	class Terrain;

	class CameraController
	{
	public:
		virtual void Tick(Camera* camera) = 0;
	};
}

namespace RTS
{
	class TopDownCameraController :
		public Framework::CameraController
	{
	public:
		TopDownCameraController(Framework::Terrain* terrain);

		void Tick(Framework::Camera* camera) override;

		struct CameraFocus
		{
			glm::vec3 mPoint{};
			float mRotation{};
			float mBezierT{};
			static constexpr inline glm::vec2 sBezierP0 = { -20.0f, 100.0f };
			static constexpr inline glm::vec2 sBezierP1 = { -50.0f, 30.0f };
			static constexpr inline glm::vec2 sBezierP2 = { -25.0f, 30.0f };
		};
		CameraFocus mFocus{};

	private:
		static glm::vec3 CalculateCameraOffset(const CameraFocus& focus, const glm::vec2& cameraForward, std::optional<float> overrideTime = {});

		// Needed for keeping the camera inside bounds and adjusting the height
		Framework::Terrain* mTerrain{};
	};
}


