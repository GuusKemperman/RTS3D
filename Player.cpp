#include "precomp.h"
#include "Player.h"

#include "Scene.h"
#include "Camera.h"
#include "Unit.h"
#include "EntityManager.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "Physics.h"
#include "Army.h"
#include "Commands.h"
#include "AssetManager.h"
#include "Mesh.h"
#include "Scope.h"
#include "TerrainData.h"
#include "Terrain.h"
#include "game.h"

#include "Explosion.h"

RTS::Player::Player(Framework::Scene& scene, Framework::EntityId armyEntityId) :
	Entity(scene),
	mCameraController(scene.mTerrain.get())
{
	mSelectedIndicatorMeshId = Framework::AssetManager::Inst().GetAsset<Framework::Mesh>("models/selectedindicator.obj")->GetMeshId();
	mHighlightedIndicatorMeshId = Framework::AssetManager::Inst().GetAsset<Framework::Mesh>("models/highlightedindicator.obj")->GetMeshId();
	mEnemyHighlightedIndicatorMeshId = Framework::AssetManager::Inst().GetAsset<Framework::Mesh>("models/enemyhighlightedindicator.obj")->GetMeshId();

	mArmy = dynamic_cast<Army*>(scene.mEntityManager->TryGetEntity(armyEntityId).value_or(nullptr));
}

void RTS::Player::Tick()
{
	mCameraController.Tick(mScene.mCamera.get());

	UpdateSelectingArea();

	mHighlightedUnits.clear();
	UpdateWhichUnitsAreSelected();
	mScene.mEntityManager->RemoveInvalidIds(mSelection);

	Framework::InputManager& input = Framework::InputManager::Inst();

	if (input.GetInput(Framework::InputId::MOUSE_RIGHT).down)
	{
		const Unit* hoveringOverEnemyUnit{};

		for (const Unit* unit : mHighlightedUnits)
		{
			if (unit->GetArmyId() != ArmyId::player)
			{
				hoveringOverEnemyUnit = unit;
				break;
			}
		}

		std::vector<Unit*> selectedUnits = mScene.mEntityManager->ConvertToType<Unit>(mSelection);

		if (hoveringOverEnemyUnit != nullptr)
		{
			for (Unit* myUnit : selectedUnits)
			{
				myUnit->GiveCommand<CommandAttack>(hoveringOverEnemyUnit->GetId());
			}
		}
		else
		{
			const std::unique_ptr<Framework::Camera>& camera = mScene.mCamera;
			const glm::vec3& cameraPosition = camera->GetTransform().GetLocalPosition();
			const glm::vec3 rayDirection = camera->CalculateRayDirection(input.GetMousePos());

			const Framework::Physics::RayCastHit hit = mScene.mPhysics->RayCast(cameraPosition, rayDirection, INFINITY);

			if (hit.mHit != nullptr)
			{
				FormFormation(std::move(selectedUnits), { hit.mPosition.x, hit.mPosition.z });
			}
		}
	}

	// Here come the sketchy and lazy quick-fixes
	Draw(); // Otherwise doesn't get drawn due to frustum culling, bit messy but that's okay for me.
}

bool RTS::Player::Serialize(Framework::Data::Scope& parentScope) const
{
	Entity::Serialize(parentScope);

	Framework::Data::Scope& myScope = parentScope.AddChild("Player");
	myScope.AddVariable("armyEntityId") << mArmy->GetId();
	myScope.AddVariable("selectedUnits") << mSelection;
	myScope.AddVariable("focusPos") << mCameraController.mFocus.mPoint;
	myScope.AddVariable("bezierT") << mCameraController.mFocus.mBezierT;
	myScope.AddVariable("rotation") << mCameraController.mFocus.mRotation;

	return true;
}

void RTS::Player::Deserialize(const Framework::Data::Scope& parentScope)
{
	Entity::Deserialize(parentScope);

	const Framework::Data::Scope& myScope = parentScope.GetScope("Player");

	Framework::EntityId armyEntityId;
	myScope.GetVariable("armyEntityId") >> armyEntityId;

	mScene.mEntityManager->DelayedIdRequest(armyEntityId,
		[](Entity& entity, void* me)
		{
			static_cast<Player*>(me)->SetArmy(static_cast<Army*>(&entity));
		}, this);

	myScope.GetVariable("selectedUnits") >> mSelection;
	myScope.GetVariable("focusPos") >> mCameraController.mFocus.mPoint;
	myScope.GetVariable("bezierT") >> mCameraController.mFocus.mBezierT;
	myScope.GetVariable("rotation") >> mCameraController.mFocus.mRotation;
}

void RTS::Player::Draw() const
{
	if (mSelectingArea.has_value())
	{
		mScene.mCamera->DrawBox(mSelectingArea.value());
	}

	const float time = Framework::TimeManager::GetTotalTimePassed();
	Framework::Transform appliedTransform{};
	appliedTransform.SetLocalScale(glm::vec3{ 1.25f });
	appliedTransform.SetLocalOrientation(0.0f, time, 0.0f);
	appliedTransform.SetLocalPosition(glm::vec3{ 0.0f, 4.0f, 0.0f });

	const glm::mat4 applyMatrix = appliedTransform.GetLocalMatrix();

	for (const Unit* unit : mHighlightedUnits)
	{
		const Framework::Transform& transform = unit->GetTransform();

		Framework::MeshId meshId = unit->GetArmyId() == mArmy->GetArmyId() ? mHighlightedIndicatorMeshId : mEnemyHighlightedIndicatorMeshId;
		mScene.mCamera->RequestInstanceDraw(meshId, transform.GetLocalMatrix() * applyMatrix);
	}

	for (const Framework::EntityId& id : mSelection)
	{
		const Unit* unit = static_cast<Unit*>(mScene.mEntityManager->TryGetEntity(id).value_or(nullptr));

		if (unit != nullptr)
		{
			const Framework::Transform& transform = unit->GetTransform();

			mScene.mCamera->RequestInstanceDraw(mSelectedIndicatorMeshId, transform.GetLocalMatrix() * applyMatrix);
		}
	}
}

void RTS::Player::UpdateWhichUnitsAreSelected()
{
	const Framework::InputManager& inp = Framework::InputManager::Inst();
	const bool shiftHeld = inp.GetInput(Framework::InputId::LEFT_SHIFT).held;
	// Storing and retrieving selections
	for (size_t keyCode = static_cast<size_t>(Framework::InputId::NORMAL_1); keyCode <= static_cast<size_t>(Framework::InputId::NORMAL_9); keyCode++)
	{
		const Framework::InputId id = static_cast<Framework::InputId>(keyCode);
		if (!inp.GetInput(id).down)
		{
			continue;
		}

		Selection& storedSelection = mStoredSelections[static_cast<size_t>(keyCode) - static_cast<size_t>(Framework::InputId::NORMAL_1) + 1];

		if (shiftHeld)
		{
			// Store the active selection
			storedSelection = mSelection;
			return;
		}

		// Retrieve a selection
		mSelection = storedSelection;
		return;
	}

	//const bool mouseHoveringOverUI = false;//scene.canvas->GetMouseHoveringOverType();
	//
	const Framework::Input& leftMouse = inp.GetInput(Framework::InputId::MOUSE_LEFT);

	//const bool pressed = leftMouse.down;
	//const bool held = leftMouse.held;
	const bool released = leftMouse.up;
	const glm::vec2 glMousePos = Framework::Math::PixelToOpenGLPosition(inp.GetMousePos());

	//if (!mSelectingArea.has_value())
	//{
	//	if (mouseHoveringOverUI)
	//	{
	//		return;
	//	}

	//	// Mouse could be hovering over entities, highlight them.
	//	std::vector<Unit*> hoveringOverUnits = CheckForUnits(glMousePos);

	//	if (pressed)
	//	{
	//		RemoveUnselectableUnits(hoveringOverUnits);
	//		mSelection = Framework::EntityManager::ConvertToEntityIds(hoveringOverUnits);
	//		mSelectingArea.reset();
	//		mSelectingAreaStart.reset();
	//	}
	//	else
	//	{
	//		mHighlightedUnits = hoveringOverUnits;
	//	}
	//	
	//	// No selecting area
	//	return;
	//}

	std::vector<Unit*> unitsInsideArea = mSelectingArea.has_value() ? CheckForUnits(mSelectingArea.value()) : CheckForUnits(glMousePos);
	mHighlightedUnits = unitsInsideArea;
	
	if (released)
	{
		// Select
		RemoveUnselectableUnits(unitsInsideArea);
		mSelection = Framework::EntityManager::ConvertToEntityIds(unitsInsideArea);
		mSelectingArea.reset();
		mSelectingAreaStart.reset();
	}
}

void RTS::Player::UpdateSelectingArea()
{
	const Framework::InputManager& inp = Framework::InputManager::Inst();
	const Framework::Input& leftMouse = inp.GetInput(Framework::InputId::MOUSE_LEFT);

	const bool hoveringOverUI = false;
	const bool pressed = leftMouse.down,
		held = leftMouse.held,
		released = leftMouse.up;

	const glm::ivec2 mousePos = inp.GetMousePos();

	if (mousePos.x < 0
		|| mousePos.x >= static_cast<int>(sScreenWidth)
		|| mousePos.y < 0
		|| mousePos.y >= static_cast<int>(sScreenHeight))
	{
		mSelectingArea.reset();
		return;
	}

	const glm::vec2 glMousePos = Framework::Math::PixelToOpenGLPosition(mousePos);

	if (pressed)
	{
		if (hoveringOverUI)
		{
			mSelectingAreaStart.reset();
		}
		else
		{
			// Start new selecting area
			mSelectingAreaStart = glMousePos;
		}
	}

	if (!mSelectingAreaStart.has_value())
	{
		mSelectingArea.reset();
		return;
	}

	if (!held && !released)
	{
		mSelectingAreaStart.reset();
		mSelectingArea.reset();
		return;
	}

	const glm::vec2 topLeft(std::min(glMousePos.x, mSelectingAreaStart.value().x), std::min(glMousePos.y, mSelectingAreaStart.value().y));
	const glm::vec2 bottomRight(std::max(glMousePos.x, mSelectingAreaStart.value().x), std::max(glMousePos.y, mSelectingAreaStart.value().y));
	const glm::vec2 size(bottomRight.x - topLeft.x, bottomRight.y - topLeft.y);

	constexpr float minTotalSize = 0.01f;
	if (size.x + size.y >= minTotalSize)
	{
		mSelectingArea = { topLeft, size };
	}
	else
	{
		mSelectingArea.reset();
	}
}

std::vector<RTS::Unit*> RTS::Player::CheckForUnits(const glm::vec2 atScreenPosition) const
{
	const std::vector<Unit*> allUnits = mScene.mEntityManager->GetEntities<Unit>();
	const glm::mat4& viewProjection = mScene.mCamera->GetViewProjection();
	constexpr float zFarInv = 1.0f / Framework::Camera::zFar;
	const float currentZoom = mScene.mCamera->GetZoom();

	Unit* bestUnit{};

	constexpr float widthMod = 1920.0f / sScreenWidth;
	constexpr float heightMod = 1080.0f / sScreenHeight;
	constexpr float avgMod = (widthMod + heightMod) * 0.5f;
	// Increasing this value increased the distance that the mouse can be away from them in clipspace, and them still getting highlighted.
	const float pointHighlightTreshold = Framework::Camera::zFar * (.05f / avgMod) * currentZoom;
	float lowestValue = pointHighlightTreshold;

	for (Unit* unit : allUnits)
	{
		const glm::vec3 scrnSpace = viewProjection * unit->GetTransform().GetWorldMatrix() * glm::vec4{ 0.0f, 0.0f, 0.0f, 1.0f };
		const glm::vec3 clipSpace = { scrnSpace.x / scrnSpace.z, scrnSpace.y / scrnSpace.z, scrnSpace.z * zFarInv };

		if (clipSpace.z <= 0.0f || clipSpace.z > 1.0f)
		{
			continue;
		}

		//// To get the radius in clip space
		//const glm::vec3 scrnSpaceOffset = viewProjection * unit->GetTransform().GetWorldMatrix() * glm::vec4{ 0.0f, unit->GetRadius(), 0.0f, 1.0f};
		//const glm::vec3 clipSpaceOffset = { scrnSpaceOffset.x / scrnSpaceOffset.z, scrnSpaceOffset.y / scrnSpaceOffset.z, scrnSpaceOffset.z / mScene.mCamera->zFar };
		//const float clipSpaceRadius = glm::distance(clipSpace, clipSpaceOffset);

		const float value = glm::distance(atScreenPosition, glm::vec2{ clipSpace }) * scrnSpace.z;

		if (value < lowestValue)
		{
			bestUnit = unit;
			lowestValue = value;
		}
	}

	return bestUnit == nullptr ? std::vector<Unit*>{} : std::vector<Unit*>{ bestUnit };
}

std::vector<RTS::Unit*> RTS::Player::CheckForUnits(const Framework::BoundingBox2D& inBox) const
{
	const std::vector<Unit*> allUnits = mScene.mEntityManager->GetEntities<Unit>();
	const glm::mat4& viewProjection = mScene.mCamera->GetViewProjection();

	std::vector<Unit*> found{};
	found.reserve(allUnits.size());

	for (Unit* unit : allUnits)
	{
		const glm::vec3 scrnSpace = viewProjection * glm::vec4{ unit->GetTransform().GetLocalPosition(), 1.0f };
		const glm::vec3 clipSpace = { scrnSpace.x / scrnSpace.z, scrnSpace.y / scrnSpace.z, scrnSpace.z / mScene.mCamera->zFar };

		if (clipSpace.z < 0.0f || clipSpace.z > 1.0f)
		{
			continue;
		}

		if (inBox.Contains(clipSpace))
		{
			found.push_back(unit);
		}
	}

	return found;
}

void RTS::Player::RemoveUnselectableUnits(std::vector<Unit*>& fromVector) const
{
	for (size_t i = 0; i < fromVector.size();)
	{
		Unit*& unit = fromVector[i];

		if (unit->GetArmyId() != mArmy->GetArmyId())
		{
			unit = fromVector.back();
			fromVector.pop_back();
		}
		else
		{
			++i;
		}
	}
}