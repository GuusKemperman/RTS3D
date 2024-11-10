#include "precomp.h"
#include "CameraControllers.h"

#include "Camera.h"
#include "InputManager.h"
#include "TimeManager.h"
#include "Terrain.h"

glm::vec3 RTS::TopDownCameraController::CalculateCameraOffset(const RTS::TopDownCameraController::CameraFocus& focus, const glm::vec2& cameraForward, std::optional<float> overrideTime)
{
	const glm::vec2 bezierOffset = Framework::Math::BezierCurve(focus.sBezierP0, focus.sBezierP1, focus.sBezierP2, overrideTime.value_or(focus.mBezierT));
	const glm::vec2 rotatedOffset = -cameraForward * bezierOffset.x;
	const glm::vec3 totalOffset = { rotatedOffset.x, bezierOffset.y, rotatedOffset.y };
	return totalOffset;
}

RTS::TopDownCameraController::TopDownCameraController(Framework::Terrain* terrain) :
	mTerrain(terrain)
{
}

void RTS::TopDownCameraController::Tick(Framework::Camera* camera)
{
	constexpr bool freecam = false;
	if (freecam)
	{
		float currentZoom = camera->GetZoom();

		float orbitAmount{};
		if (Framework::InputManager::GetInput(Framework::InputId::Q).held)
		{
			orbitAmount++;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::E).held)
		{
			orbitAmount--;
		}

		float rotationY{};
		if (Framework::InputManager::GetInput(Framework::InputId::ARROW_LEFT).held)
		{
			rotationY++;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::ARROW_RIGHT).held)
		{
			rotationY--;
		}

		float rotationX{};
		if (Framework::InputManager::GetInput(Framework::InputId::ARROW_DOWN).held)
		{
			rotationX++;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::ARROW_UP).held)
		{
			rotationX--;
		}

		Framework::Transform& cameraTransform = camera->GetTransform();
		const float deltaTime = Framework::TimeManager::GetDeltaTime();

		constexpr float rotationSpeed = 1.0f;
		const float rotationScalar = (rotationSpeed / currentZoom) * deltaTime;

		glm::quat orientation = cameraTransform.GetLocalOrientation();
		const glm::quat rotation = glm::vec3{ 0.0f, (orbitAmount + rotationY) * rotationScalar, 0.0f };
		const glm::quat rotationAroundX = glm::vec3{ rotationX * rotationScalar, 0.0f, 0.0f };

		orientation = rotation * orientation * rotationAroundX;
		cameraTransform.SetLocalOrientation(orientation);

		constexpr float movementSpeed = 50.0f;
		constexpr float slownessMultiplier = 0.25f;
		const bool slownessButtonHeld = Framework::InputManager::GetInput(Framework::InputId::LEFT_SHIFT).held;

		const float movementScalar = (slownessButtonHeld ? slownessMultiplier : 1.0f) * (movementSpeed / currentZoom) * deltaTime;

		float forwardMovement{};
		if (Framework::InputManager::GetInput(Framework::InputId::W).held)
		{
			forwardMovement++;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::S).held)
		{
			forwardMovement--;
		}

		const glm::vec3 forward = cameraTransform.GetLocalForward();
		const glm::vec2 forward2D = normalize(glm::vec2{ forward.x, forward.z });
		cameraTransform.TranslateLocalPosition(forward2D * forwardMovement * movementScalar);

		float rightMovement = orbitAmount * rotationSpeed;
		if (Framework::InputManager::GetInput(Framework::InputId::A).held)
		{
			rightMovement--;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::D).held)
		{
			rightMovement++;
		}

		const glm::vec3 right = cameraTransform.GetLocalRight();
		const glm::vec2 right2D = normalize(glm::vec2{ right.x, right.z });
		cameraTransform.TranslateLocalPosition(right2D * rightMovement * movementScalar);

		float verticalMovement{};
		if (Framework::InputManager::GetInput(Framework::InputId::SPACE).held)
		{
			verticalMovement++;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::LEFT_CTRL).held)
		{
			verticalMovement--;
		}

		cameraTransform.TranslateLocalPosition(0.0f, verticalMovement * movementScalar, 0.0f);

		return;
	}


	const float deltaTime = Framework::TimeManager::GetDeltaTime();

	{ // Rotation
		float orbitAmount{};
		if (Framework::InputManager::GetInput(Framework::InputId::Q).held)
		{
			orbitAmount++;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::E).held)
		{
			orbitAmount--;
		}

		constexpr float rotationSpeed = 100.0f;
		const float rotationScalar = rotationSpeed * deltaTime;
		mFocus.mRotation = fmodf(mFocus.mRotation + orbitAmount * rotationScalar, 360.0f);
	}

	{
		float verticalMovement{};
		if (Framework::InputManager::GetInput(Framework::InputId::SPACE).held)
		{
			verticalMovement--;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::LEFT_CTRL).held)
		{
			verticalMovement++;
		}
		constexpr float zoomSensitivity = 1.0f;
		mFocus.mBezierT = std::clamp(mFocus.mBezierT + verticalMovement * zoomSensitivity * deltaTime, 0.0f, 1.0f);
	}

	const glm::vec2 cameraForward = Framework::Math::AngleToVec2(mFocus.mRotation);
	{
		constexpr float movementSpeed = 50.0f;
		constexpr float sprintMultiplier = 5.0f;
		const bool sprintButtonHeld = Framework::InputManager::GetInput(Framework::InputId::LEFT_SHIFT).held;
		const float movementScalar = (sprintButtonHeld ? sprintMultiplier : 1.0f) * movementSpeed * deltaTime;

		float forwardScalar{};
		if (Framework::InputManager::GetInput(Framework::InputId::W).held)
		{
			forwardScalar--;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::S).held)
		{
			forwardScalar++;
		}

		const glm::vec2 forward2D = cameraForward * forwardScalar;

		float rightScalar{};
		if (Framework::InputManager::GetInput(Framework::InputId::A).held)
		{
			rightScalar++;
		}
		if (Framework::InputManager::GetInput(Framework::InputId::D).held)
		{
			rightScalar--;
		}

		const glm::vec2 cameraRight = { -cameraForward.y, cameraForward.x };
		const glm::vec2 right2D = cameraRight * rightScalar;

		const glm::vec3 velocity = glm::vec3{ forward2D.x + right2D.x, 0.0f, forward2D.y + right2D.y } *movementScalar;
		mFocus.mPoint += velocity;

		// Clamp it to be inside the terrain
		Framework::TerrainData* data = mTerrain->GetData();
		mFocus.mPoint.x = std::clamp(mFocus.mPoint.x, 0.0f, data->mWorldSizeX);
		mFocus.mPoint.z = std::clamp(mFocus.mPoint.z, 0.0f, data->mWorldSizeZ);

		// And adjust the height
		mFocus.mPoint.y = data->GetHeightAtPosition(mFocus.mPoint.x, mFocus.mPoint.z);
	}

	// Generating a 3D point and orientation based on what we're focusing on
	{
		Framework::Transform& cameraTransform = camera->GetTransform();

		//ImGui::SetNextWindowPos({ 1500, 700 });
		//ImGui::SetNextWindowSize({ 400, 400 });
		//ImGui::SetNextWindowBgAlpha(1.0f); 
		//if(ImGui::Begin("Testfadf"))
		//{
		//	ImGui::InputFloat2("p0", &mFocus.sBezierP0[0]);
		//	ImGui::InputFloat2("p1", &mFocus.sBezierP1[0]);
		//	ImGui::InputFloat2("p2", &mFocus.sBezierP2[0]);
		//}
		//ImGui::End();

		const glm::vec3 totalOffset = CalculateCameraOffset(mFocus, cameraForward);

		cameraTransform.SetLocalPosition(mFocus.mPoint + totalOffset);

		const glm::vec3 derivative = CalculateCameraOffset(mFocus, cameraForward, mFocus.mBezierT + 0.01f);

		cameraTransform.SetLocalOrientation(glm::quatLookAt(glm::normalize(derivative), { 0.0f, 1.0f, 0.0f }));
	}
}