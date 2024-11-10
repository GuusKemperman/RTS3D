#include "precomp.h"
#include "Turret.h"

#include "AssetManager.h"
#include "Mesh.h"
#include "Scene.h"
#include "EntityManager.h"
#include "TimeManager.h"
#include "Unit.h"
#include "Projectile.h"
#include "Physics.h"
#include "Frustum.h"
#include "Scope.h"
#include "ProceduralUnitFactory.h"
#include "Unit.h"

RTS::Turret::Turret(Framework::Scene& scene, const TurretData* turretData, Unit* attachTo, std::optional<Framework::Transform> nodeOnUnitBody) :
	Entity(scene)
{
	mHasFixedTick = true;
	mInquirer.m_collisionFilterGroup = Framework::Physics::Group::projectileGroup;
	mInquirer.m_collisionFilterMask = Framework::Physics::Group::unitGroup; // Only check for units

	if (turretData != nullptr)
	{
		SetTurretData(turretData);
	}

	if (attachTo != nullptr)
	{
		mAttachedToNode = nodeOnUnitBody.value();
		mAttachedToNode.SetParent(&attachTo->GetTransform());
	}

	GetTransform().SetParent(&mAttachedToNode);
}

void RTS::Turret::Tick()
{
	const float deltaTime = Framework::TimeManager::GetDeltaTime();

	Framework::Transform& myTransform = GetTransform();

	const glm::vec3 parentForward = myTransform.GetParent()->GetWorldForward();
	glm::vec3 desiredAimDirection = parentForward;

	const glm::vec3 myPosition = myTransform.GetWorldPosition();

	const Framework::Entity* target{};

	if (mDesiredTarget.has_value())
	{
		// Check if target is still valid.
		target = mScene.mEntityManager->TryGetEntity(mDesiredTarget.value()).value_or(nullptr);
		if (target == nullptr)
		{
			mDesiredTarget.reset();
			desiredAimDirection = parentForward;
		}
		else
		{
			const Framework::Transform& targetTransform = target->GetTransform();
			glm::vec3 targetPosition = targetTransform.GetLocalPosition();

			if (targetPosition != myPosition)
			{
				desiredAimDirection = glm::normalize(targetPosition - myPosition);
			}
		}
	}
	else if (mLockedOntoTarget.has_value())
	{
		mLockedOntoTarget.reset();
	}
	
	const glm::quat desiredOrientation = Framework::Transform::CalculateRotationBetweenOrientations(glm::vec3{ 0.0f, 0.0f, 1.0f }, desiredAimDirection);

	const glm::quat myOrientation = myTransform.GetWorldOrientation();
	const float maxTurn = mTurretData->mTurnSpeed * deltaTime;

	const glm::quat orientationAfterTurning = Framework::Transform::CalculateOrientationTowards(myOrientation, desiredOrientation, maxTurn);
	assert(!isnan(orientationAfterTurning.x));

	myTransform.SetWorldOrientation(orientationAfterTurning);

	// Firing logic
	if (target == nullptr)
	{
		return;
	}

	const float dot = glm::dot(desiredOrientation, orientationAfterTurning);

	if (dot < .9f)
	{
		mLockedOntoTarget.reset();
	}
	else if (mLockedOntoTarget.value_or(-1) != mDesiredTarget.value())
	{
		LockOnto(mDesiredTarget.value());
	}

	// Return if lock not complete.
	const float currentTime = Framework::TimeManager::GetTotalTimePassed();
	if (currentTime - mTimeLastFired < mTurretData->mCooldown
		|| currentTime - mTimeLockStart < mTurretData->mCooldown * 0.5f)
	{
		return;
	}
	
	const glm::vec3 forward = myTransform.GetWorldForward();
	const glm::vec3 fireFromPosition = myTransform.GetWorldPosition() + forward * 5.0f;
	const glm::vec3 velocity = forward * 100.0f;

	mScene.mEntityManager->AddEntity<Projectile>(fireFromPosition, velocity, mTurretData->mDamage * mAttachedToNode.GetLocalScale().x);

	mTimeLastFired = currentTime;
}

void RTS::Turret::FixedTick()
{
	const Framework::Transform& myTransform = GetTransform();

	const Unit* owner = static_cast<Unit*>(mAttachedToNode.GetParent()->GetOwner());
	assert(owner != nullptr);
	//if (mLockedOntoTarget.has_value())
	//{
	//	// If the target that is currently being locked on has not been destroyed yet, use it.
	//	Entity* target = mScene.mEntityManager->TryGetEntity(mLockedOntoTarget.value()).value_or(nullptr);

	//	if (target != nullptr
	//		&& IsInSights(target))
	//	{
	//		mDesiredTarget = mLockedOntoTarget;
	//		return;
	//	}
	//	mLockedOntoTarget.reset();
	//}
	
	const glm::vec2 myPosition2D = myTransform.GetWorldPosition();

	const ArmyId myArmyId = owner->GetArmyId();

	const Unit* closestTarget{};
	float closestTargetDistance2 = INFINITY;

	mScene.mPhysics->Query(mInquirer, mAttachedToNode);

	for (const btCollisionObject* object : mInquirer.mCollidedWith)
	{
		if (object->getUserPointer() == nullptr)
		{
			continue;
		}

		assert(dynamic_cast<Unit*>(static_cast<Entity*>(object->getUserPointer())) != nullptr
			&& "Object owner was not a unit");
		const Unit* potentialTarget = static_cast<const Unit*>(object->getUserPointer());

		if (potentialTarget == nullptr
			|| potentialTarget->GetArmyId() == myArmyId)
		{
			continue;
		}

		const float distance2 = glm::distance2(myPosition2D, potentialTarget->GetTransform().GetLocalPosition2D());

		if (distance2 < closestTargetDistance2)
		{
			closestTarget = potentialTarget;
			closestTargetDistance2 = distance2;
		}
	}

	if (closestTarget == nullptr)
	{
		mDesiredTarget.reset();
	}
	else
	{
		mDesiredTarget = closestTarget->GetId();
	}
}

bool RTS::Turret::Serialize(Framework::Data::Scope& parentScope) const
{
	Entity::Serialize(parentScope);

	Framework::Data::Scope& myScope = parentScope.AddChild("Turret");

	Framework::Data::Scope& nodeScope = myScope.AddChild("attachedToNode");
	mAttachedToNode.Serialize(nodeScope);

	assert(mTurretData != nullptr);
	myScope.AddVariable("dataIndex") << static_cast<uchar>(mTurretData->mIndexInFactory);

	const float currentTime = Framework::TimeManager::GetTotalTimePassed();
	myScope.AddVariable("timeLeftOnCooldown") << mTurretData->mCooldown - currentTime - mTimeLastFired;
	myScope.AddVariable("timeLeftOnLock") << mTurretData->mCooldown - currentTime - mTimeLockStart;

	if (mDesiredTarget.has_value())
	{
		myScope.AddVariable("desiredTarget") << mDesiredTarget.value();
	}
	if (mLockedOntoTarget.has_value())
	{
		myScope.AddVariable("lockedOnto") << mLockedOntoTarget.value();
	}

	return true;
}

void RTS::Turret::Deserialize(const Framework::Data::Scope& parentScope)
{
	Entity::Deserialize(parentScope);

	const Framework::Data::Scope& myScope = parentScope.GetScope("Turret");

	const Framework::Data::Scope& nodeScope = myScope.GetScope("attachedToNode");
	mAttachedToNode.Deserialize(nodeScope, mScene);
	
	uchar dataIndex;
	myScope.GetVariable("dataIndex") >> dataIndex;
	const TurretData* turretData = Framework::AssetManager::Inst().GetAsset<ProceduralUnitFactory>(sDataRootWithoutAssetRoot + "procedural")->GetTurret(dataIndex);
	SetTurretData(turretData);

	const float currentTime = Framework::TimeManager::GetTotalTimePassed();
	float timeLeftOnCooldown;
	myScope.GetVariable("timeLeftOnCooldown") >> timeLeftOnCooldown;
	mTimeLastFired = timeLeftOnCooldown - mTurretData->mCooldown + currentTime;

	float timeLeftOnLock;
	myScope.GetVariable("timeLeftOnLock") >> timeLeftOnLock;
	mTimeLockStart = timeLeftOnLock - mTurretData->mCooldown + currentTime;

	std::optional<Framework::Data::Variable> desiredTarget = myScope.TryGetVariable("desiredTarget");
	if (desiredTarget.has_value())
	{
		Framework::EntityId target;
		desiredTarget.value() >> target;
		mDesiredTarget = target;
	}

	std::optional<Framework::Data::Variable> lockedOnto = myScope.TryGetVariable("lockedOnto");
	if (lockedOnto.has_value())
	{
		Framework::EntityId lockedOn;
		lockedOnto.value() >> lockedOn;
		mLockedOntoTarget = lockedOn;
	}
}

void RTS::Turret::SetTurretData(const TurretData* turretData)
{
	assert(turretData != nullptr);
	mTurretData = turretData;

	mInquirer.mCollisionObject.setCollisionShape(mTurretData->mCanFireWithinShape.get());
	mMeshId = mTurretData->mMeshId;
}

void RTS::Turret::LockOnto(Framework::EntityId id)
{
	mLockedOntoTarget = id;
	mTimeLockStart = Framework::TimeManager::GetTotalTimePassed();
}