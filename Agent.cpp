#include "precomp.h"
#include "Agent.h"

#include "Scope.h"
#include "Mesh.h"
#include "AssetManager.h"
#include "TimeManager.h"
#include "Scene.h"
#include "Terrain.h"
#include "Camera.h"
#include "Physics.h"

Framework::Agent::Agent(Scene& scene) :
	Entity(scene)
{
	mHasFixedTick = true; 

	static btSphereShape avoidanceSphere = { sAvoidanceRange };

	mAvoidanceInquirer.mCollisionObject.setCollisionShape(&avoidanceSphere);
	mAvoidanceInquirer.mCollisionObject.setCollisionShape(&avoidanceSphere);
	mAvoidanceInquirer.mCollisionObject.setCollisionFlags(mAvoidanceInquirer.mCollisionObject.getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

	mAvoidanceInquirer.m_collisionFilterGroup = Physics::Group::agentGroup;
	mAvoidanceInquirer.m_collisionFilterMask = Physics::Group::agentGroup | Physics::Group::staticObstacleGroup; // Only care about avoiding these
}

Framework::Agent::~Agent() = default;

void Framework::Agent::Tick()
{
	Transform& myTransform = GetTransform();

	btRigidBody* myRigidBody = static_cast<btRigidBody*>(GetCollisionObject());
	myRigidBody->activate(true);

	if (IsInRagdollState())
	{
		return;
	}

	const glm::vec3 currentPosition = myTransform.GetLocalPosition();

	glm::quat desiredOrientation = CalculateDesideredOrientation();
	const glm::quat currentOrientation = myTransform.GetLocalOrientation();

	const glm::quat orientationAfterTurning = Transform::CalculateOrientationTowards(currentOrientation, desiredOrientation, .25f);
	
	const glm::quat rotationToMake = Transform::CalculateRotationBetweenOrientations(currentOrientation, orientationAfterTurning) * mTurnSpeed * 3.0f;
	const glm::vec3 angularVelocity = glm::eulerAngles(rotationToMake);

#ifdef DEBUG
	{
		const glm::vec3 desiredForward = Transform::RotateVector(glm::vec3{ 0.0f, 0.0f, 1.0f }, desiredOrientation);
		const glm::vec3 afterTurning = Transform::RotateVector(glm::vec3{ 0.0f, 0.0f, 1.0f }, orientationAfterTurning);
		const glm::vec3 currentForward = myTransform.GetLocalForward();

		mScene.mCamera->RequestDebugLineDraw(currentPosition, currentPosition + currentForward * sAvoidanceRange, { 0.0f, 0.0f, 1.0f });
		mScene.mCamera->RequestDebugLineDraw(currentPosition, currentPosition + afterTurning   * sAvoidanceRange, { 0.0f, 1.0f, 0.0f });
		mScene.mCamera->RequestDebugLineDraw(currentPosition, currentPosition + desiredForward * sAvoidanceRange, { 1.0f, 0.0f, 0.0f });
	}
#endif // DEBUG

	myRigidBody->setAngularVelocity(Math::ToBullet(angularVelocity));
	float velocityScalar = glm::dot(currentOrientation, desiredOrientation);

	glm::vec3 desiredPosition = currentPosition;
	glm::vec3 forward = myTransform.GetLocalForward();

	desiredPosition += glm::length(mLastInput.mDesiredVelocity * velocityScalar) * forward;

	const std::unique_ptr<Terrain>& terrain = mScene.mTerrain;

	desiredPosition.x = std::clamp(desiredPosition.x, 0.0f, terrain->GetWorldSizeX());
	desiredPosition.y = CalculateDesiredHeight({ currentPosition.x, currentPosition.z });
	desiredPosition.z = std::clamp(desiredPosition.z, 0.0f, terrain->GetWorldSizeZ());

	mVelocity = desiredPosition - currentPosition;
	
	myRigidBody->setLinearVelocity(Math::ToBullet(mVelocity));
}

float Framework::Agent::CalculateDesiredHeight(const glm::vec2& atPosition) const
{
	const std::unique_ptr<Terrain>& terrain = mScene.mTerrain;
	float terrainHeight = terrain->GetHeightAtPosition(atPosition.x, atPosition.y);
	return terrainHeight + mHoverAtHeight;
}

glm::vec2 Framework::Agent::CombineVelocities(const glm::vec2& dominantVelocity, const glm::vec2& recessiveVelocity)
{
	const float oldRecessiveLength = glm::length(recessiveVelocity);

	if (oldRecessiveLength == 0.0f) // Prevents divide by zero
	{
		return dominantVelocity;
	}

	// Floating point errors require the min..
	const float dominantLength = std::min(glm::length(dominantVelocity), 1.0f);

	const float newRecessiveLength = std::min(1.0f - dominantLength, oldRecessiveLength);
	const glm::vec2 newRecessive = (recessiveVelocity / oldRecessiveLength) * newRecessiveLength;

	return dominantVelocity + newRecessive;
}

glm::quat Framework::Agent::CalculateDesideredOrientation() const
{
	const Transform& myTransform = GetTransform();
	glm::vec3 desiredForward{};
	
	if (mLastInput.mDesiredVelocity != glm::vec2{ 0.0f, 0.0f })
	{
		desiredForward = { mLastInput.mDesiredVelocity.x, 0.0f, mLastInput.mDesiredVelocity.y };
	}
	else if (mLastInput.mDesiredForward.has_value())
	{
		desiredForward = { mLastInput.mDesiredForward.value().x, 0.0f, mLastInput.mDesiredForward.value().y };
	}
	else
	{
		desiredForward = myTransform.GetLocalForward();
	}

	const glm::quat newForward = Transform::CalculateRotationBetweenOrientations(glm::vec3{ 0.0f, 0.0f, 1.0f }, desiredForward);

	const std::unique_ptr<Terrain>& terrain = mScene.mTerrain;
	const glm::vec3 currentPosition = myTransform.GetLocalPosition();

	const glm::vec3 desiredUp = terrain->GetNormalAtPosition(currentPosition.x, currentPosition.z);

	const glm::vec3 newUp = newForward * glm::vec3{ 0.0f, 1.0f, 0.0f };
	const glm::quat rotation = Transform::CalculateRotationBetweenOrientations(newUp, desiredUp);
	
	return rotation * newForward;
}

void Framework::Agent::FixedTick()
{
	assert(GetTransform().IsOrphan() 
		&& "An agent cannot have a parent object.");

	mLastInput = CalculateDesiredVelocity();

	const float length2 = glm::length2(mLastInput.mDesiredVelocity);
	mLastInput.mDesiredVelocity = (length2 > 1.0f ? mLastInput.mDesiredVelocity / sqrtf(length2) : mLastInput.mDesiredVelocity) * mMovementSpeed;
}

void Framework::Agent::ForceSetPosition(const glm::vec2 position)
{
	const glm::vec3 position3D = { position.x, CalculateDesiredHeight(position), position.y };

	Transform& myTransform = GetTransform();
	myTransform.SetLocalPosition(position3D);

	btRigidBody* rb = dynamic_cast<btRigidBody*>(GetCollisionObject());
	assert(rb != nullptr);
	rb->activate();
	rb->setMotionState(&myTransform);
	rb->activate();
}

bool Framework::Agent::Serialize(Framework::Data::Scope& parentScope) const
{
	Entity::Serialize(parentScope);

	Data::Scope& myScope = parentScope.AddChild("Agent");

	myScope.AddVariable("dVel") << mLastInput.mDesiredVelocity;
	myScope.AddVariable("vel") << mVelocity;

	return true;
}

void Framework::Agent::Deserialize(const Framework::Data::Scope& parentScope)
{
	Entity::Deserialize(parentScope);

	const Data::Scope& myScope = parentScope.GetScope("Agent");
	myScope.GetVariable("dVel") >> mLastInput.mDesiredVelocity;
	myScope.GetVariable("vel") >> mVelocity;
}

bool Framework::Agent::IsInRagdollState() const
{
	const btRigidBody* myRigidBody = dynamic_cast<btRigidBody*>(GetCollisionObject());
	if (myRigidBody == nullptr)
	{
		return false;
	}

	return myRigidBody->getLinearVelocity().length() > glm::length(mVelocity) + .5f;
}

glm::vec2 Framework::Agent::CalculateAvoidance()
{
	const Transform& myTransform = GetTransform();

	mScene.mPhysics->Query(mAvoidanceInquirer, myTransform);

	const glm::vec2 myPosition2D = GetTransform().GetLocalPosition2D();

	glm::vec2 avoidanceVelocity{};

	for (const btCollisionObject* obstacle : mAvoidanceInquirer.mCollidedWith)
	{
		// Ignore myself
		if (obstacle->getUserPointer() == this)
		{
			continue;
		}

		const glm::vec3 obstaclePosition = Math::ToGLM(obstacle->getWorldTransform().getOrigin());
		const glm::vec2 obstaclePosition2D = { obstaclePosition.x, obstaclePosition.z };
		glm::vec2 deltaPos = myPosition2D - obstaclePosition2D;

		float deltaPosLength = length(deltaPos);

		// Prevents division by 0
		if (deltaPosLength == 0.0f)
		{
			deltaPos = glm::vec2{ 0.01f };
			deltaPosLength = length(deltaPos);
		}

		const float avoidanceStrength = std::clamp((1.0f - (deltaPosLength / sAvoidanceRange)), 0.0f, 1.0f);

		avoidanceVelocity += (deltaPos / deltaPosLength) * avoidanceStrength;
	}

	if (glm::length2(avoidanceVelocity) > 1.0f)
	{
		avoidanceVelocity = normalize(avoidanceVelocity);
	}

	return avoidanceVelocity;
}

glm::vec2 Framework::Agent::CalculateSeek(const glm::vec2 towardPosition) const
{
	const glm::vec2 myPosition = GetTransform().GetLocalPosition2D();
	const glm::vec2 deltaPosition = towardPosition - myPosition;

	const float distSqrd = glm::length2(deltaPosition);
	return distSqrd == 0.0f ? glm::vec3{} : deltaPosition / (sqrtf(distSqrd));
}

glm::vec2 Framework::Agent::CalculateArrival(const glm::vec2 arriveAt) const
{
	const glm::vec2 myCentre = GetTransform().GetLocalPosition2D();
	const glm::vec2 targetOffset = arriveAt - myCentre;
	const float distToTarget = length(targetOffset);

	if (distToTarget <= 0.05f)
	{
		return glm::vec2{};
	}

	constexpr float slowingDist = 1.0f;
	const float scalar = std::min(distToTarget / slowingDist, 1.0f);

	return (targetOffset / distToTarget) * scalar;
}

glm::vec2 Framework::Agent::CalculateWander() const
{
	const glm::vec2 wanderOffset = sFixedStepSize * glm::vec2{ Random::Range(-sWanderChangeSensitivity, sWanderChangeSensitivity), Random::Range(-sWanderChangeSensitivity, sWanderChangeSensitivity) };
	return glm::normalize(glm::vec2{ mVelocity.x, mVelocity.z } + wanderOffset);
}

float Framework::Agent::CalculateAmountOfTraction() const
{
	const Transform& myTransform = GetTransform();

	const glm::vec3 myUp = myTransform.GetLocalUp();
	const glm::vec3 myPosition = myTransform.GetLocalPosition();

	const glm::vec3 terrainUp = mScene.mTerrain->GetNormalAtPosition(myPosition.x, myPosition.z);

	const float angleFactor = std::max(glm::dot(myUp, terrainUp), 0.0f);

	const float desiredHeight = CalculateDesiredHeight({ myPosition.x, myPosition.z });
	
	constexpr float scale = 5.0f;
	float heightFactor = 1.0f - std::clamp(myPosition.y - desiredHeight, 0.0f, scale) * (1.0f / scale);

	return angleFactor * heightFactor;
}