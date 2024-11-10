#include "precomp.h"
#include "Unit.h"

#include "Army.h"
#include "Scene.h"
#include "EntityManager.h"
#include "Physics.h"
#include "Turret.h"
#include "TimeManager.h"
#include "Scope.h"
#include "ProceduralUnitFactory.h"
#include "AssetManager.h"
#include "Explosion.h"

RTS::Unit::Unit(Framework::Scene& scene, Army* army) :
	Agent(scene)
{
	SetArmy(army);
	GiveCommand<CommandIdle>();
	
	static btSphereShape sightSphere = { sSightRange };

	mSightInquirer.mCollisionObject.setCollisionShape(&sightSphere);
	mSightInquirer.mCollisionObject.setCollisionFlags(mSightInquirer.mCollisionObject.getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

	mSightInquirer.m_collisionFilterGroup = Framework::Physics::Group::unitGroup;
	mSightInquirer.m_collisionFilterMask = Framework::Physics::Group::unitGroup; // Only care about other units
}

RTS::Unit::~Unit()
{
	if (mCollisionObject != nullptr)
	{
		mScene.mPhysics->RemoveCollisionObjectFromWorld(std::move(mCollisionObject));
	}
}

void RTS::Unit::SetUnitBodyData(const UnitBodyData* data, const std::optional<ArmyId> useArmyId)
{
	assert(data != nullptr);

	assert(mUnitBodyData == nullptr
		&& "The data has already been set");

	mUnitBodyData = data;
	mMeshId = data->GetMeshId(useArmyId.has_value() ? useArmyId.value() : mArmy->GetArmyId());
	mHoverAtHeight = data->mHoverAtHeight;
	mMovementSpeed = data->mMovementSpeed;
	mTurnSpeed = data->mTurnSpeed;
	mHealth = data->mMaxHealth;

	assert(mCollisionObject == nullptr
		&& "There's already a collisionobject, don't make a new one without cleaning up nicely first");


	mCollisionObject = std::make_unique<btRigidBody>(data->mMass, &GetTransform(), data->mShape.get(), data->mInertia);
	mCollisionObject->setUserPointer(this);

	mScene.mPhysics->AddCollisionObjectToWorld(mCollisionObject.get(), Framework::Physics::Group::unitGroup, Framework::Physics::Mask::unitMask);
}

void RTS::Unit::Tick()
{
	Agent::Tick();

	if ((mHealth <= 0.0f
		&& !IsInRagdollState())
		|| GetTransform().GetLocalPosition().y < 0.0f)
	{
		mScene.mEntityManager->AddEntity<Explosion>(GetTransform(), mUnitBodyData->mDeathExplosionSize);
		Destroy();
	}
}

Framework::Agent::AgentInput RTS::Unit::CalculateDesiredVelocity()
{
	Framework::Agent::AgentInput agentInput;
	do
	{
		mSwitchedState = false;
		agentInput = mCommand->CalculateAgentInput(this);
	} while (mSwitchedState); // We need to recalculate our input if we changed our state.

	return agentInput;
}

std::optional<RTS::Unit*> RTS::Unit::CheckForUnitToAttack()
{
	std::vector<RTS::Unit*> nearbyUnits = GetUnitsInSight();

	const ArmyId myArmyId = GetArmyId();
	const glm::vec2 myPosition2D = GetTransform().GetLocalPosition2D();

	RTS::Unit* target = nullptr;
	float targetDistance2 = INFINITY;

	for (RTS::Unit* possibleTarget : nearbyUnits)
	{
		if (possibleTarget->GetArmyId() == myArmyId)
		{
			continue;
		}
		
		const float distance2 = glm::distance2(myPosition2D, possibleTarget->GetTransform().GetLocalPosition2D());

		if (distance2 < targetDistance2)
		{
			target = possibleTarget;
			targetDistance2 = distance2;
		}
	}

	if (target)
	{
		return target;
	}
	
	return std::optional<RTS::Unit*>();
}

std::vector<RTS::Unit*> RTS::Unit::GetUnitsInSight()
{
	mScene.mPhysics->Query(mSightInquirer, GetTransform());

	if (mSightInquirer.mCollidedWith.empty())
	{
		return {};
	}

	std::vector<RTS::Unit*> units;
	// We will always atleast collide with ourselves
	units.reserve(mSightInquirer.mCollidedWith.size() - 1);

	for (size_t i = 0; i < mSightInquirer.mCollidedWith.size(); i++)
	{
		const btCollisionObject* object = mSightInquirer.mCollidedWith[i];
		assert(object->getUserPointer() != nullptr
			&& "Object does not have an owner");

		assert(dynamic_cast<Unit*>(static_cast<Entity*>(object->getUserPointer())) != nullptr
			&& "Object owner was not a unit");

		Unit* owner = static_cast<Unit*>(object->getUserPointer());

		if (owner != this)
		{
			units.push_back(owner);
		}
	}

	return units;
}

RTS::ArmyId RTS::Unit::GetArmyId() const
{
	return mArmy->GetArmyId();
}

void RTS::Unit::SetArmy(Army* army)
{
	mArmy = army;
}

void RTS::Unit::ReceiveDamage(float byAmount)
{
	if (mHealth <= 0.0f)
	{
		return;
	}

	mHealth -= byAmount;

	// Don't destroy it here, we'll first let it be a ragdoll for a bit. We'll check the heatlh in tick/fixedtick, and destroy it if not in a ragdoll state.
}

bool RTS::Unit::Serialize(Framework::Data::Scope& parentScope) const
{
	Agent::Serialize(parentScope);

	Framework::Data::Scope& myScope = parentScope.AddChild("Unit");
	myScope.AddVariable("bodyIndex") << static_cast<uchar>(mUnitBodyData->mIndexInFactory);
	myScope.AddVariable("commandType") << static_cast<uchar>(mCommand->GetType());
	myScope.AddVariable("aggroLevel") << static_cast<uchar>(mAggroLevel);
	myScope.AddVariable("armyId") << static_cast<uchar>(mArmy->GetArmyId());
	myScope.AddVariable("health") << mHealth;
	myScope.AddVariable("armyEntityId") << mArmy->GetId();

	// Quick solution, would be cleaner to have this as parts of the commands, with virtual serialize and deserialize them with factories.
	switch (mCommand->GetType())
	{
	case CommandType::idle:
		break;
	case CommandType::moveTo:
	{
		const CommandMoveTo* moveTo = static_cast<const CommandMoveTo*>(mCommand.get());
		myScope.AddVariable("toPos") << moveTo->mToPosition;
		if (moveTo->mDesiredForward.has_value())
		{
			myScope.AddVariable("desiredForward") << moveTo->mDesiredForward.value();
		}
		break;
	}
	case CommandType::attack:
		myScope.AddVariable("targetId") << static_cast<const CommandAttack*>(mCommand.get())->mTarget;
		break;
	}

	return true;
}

void RTS::Unit::Deserialize(const Framework::Data::Scope& parentScope)
{
	// Deserialize this first to ensure we have a rigidbody in place for the Entity class to deserialize to.
	const Framework::Data::Scope& myScope = parentScope.GetScope("Unit");

	uchar armyId;
	myScope.GetVariable("armyId") >> armyId;

	uchar bodyIndex;
	myScope.GetVariable("bodyIndex") >> bodyIndex;
	const UnitBodyData* bodyData = Framework::AssetManager::Inst().GetAsset<ProceduralUnitFactory>(sDataRootWithoutAssetRoot + "procedural")->GetBody(bodyIndex);
	SetUnitBodyData(bodyData, static_cast<ArmyId>(armyId));

	Agent::Deserialize(parentScope);

	uchar tmpType;
	myScope.GetVariable("commandType") >> tmpType;
	const CommandType type = static_cast<CommandType>(tmpType);

	switch (type)
	{
	case CommandType::idle:
		GiveCommand<CommandIdle>();
		break;
	case CommandType::moveTo:
	{
		glm::vec2 toPos;
		myScope.GetVariable("toPos") >> toPos;

		const std::optional<Framework::Data::Variable> desiredForwwardVar = myScope.TryGetVariable("desiredForward");
		std::optional<glm::vec2> desiredForward{};

		if (desiredForwwardVar.has_value())
		{
			glm::vec2 tmpDesiredForward;
			desiredForwwardVar.value() >> tmpDesiredForward;
			desiredForward = tmpDesiredForward;
		}

		GiveCommand<CommandMoveTo>(toPos, desiredForward);
		break;
	}
	case CommandType::attack:
	{
		Framework::EntityId target;
		myScope.GetVariable("targetId") >> target;
		GiveCommand<CommandAttack>(target);
		break;
	}
	}

	uchar tmpAggro;
	myScope.GetVariable("aggroLevel") >> tmpAggro;
	mAggroLevel = static_cast<AggroLevel>(tmpAggro);
	myScope.GetVariable("health") >> mHealth;

	Framework::EntityId armyEntityId;
	myScope.GetVariable("armyEntityId") >> armyEntityId;

	mScene.mEntityManager->DelayedIdRequest(armyEntityId,
		[](Entity& entity, void* me)
		{
			static_cast<Unit*>(me)->SetArmy(dynamic_cast<Army*>(&entity));
		}, this);
}
