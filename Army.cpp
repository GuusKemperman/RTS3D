#include "precomp.h"
#include "Army.h"

#include "Scene.h"
#include "Scope.h"
#include "Commands.h"
#include "Unit.h"
#include "ProceduralUnitFactory.h"
#include "AssetManager.h"

RTS::Army::Army(Framework::Scene& scene, ArmyId id) :
	Entity(scene),
	mArmyId(id)
{
	mProceduralUnitFactory = Framework::AssetManager::Inst().GetAsset<ProceduralUnitFactory>(sDataRootWithoutAssetRoot + "procedural");
}

void RTS::Army::SpawnUnits(const Framework::Data::Scope& loadFrom)
{
	const std::vector<Framework::Data::Scope>& groups = loadFrom.GetChildren();

	std::string type;
	uint amount;

	for (const Framework::Data::Scope& group : groups)
	{
		group.GetVariable("amount") >> amount;

		const std::vector<Framework::Data::Variable>& bodyFrequencies = group.GetScope("bodies").GetVariables();
		std::vector<ProceduralUnitFactory::CumulativeFrequency> cumulativeBodyFrequencies{};
		float totalFrequency = 0.0f;
		for (const Framework::Data::Variable& bodyFrequencyVar : bodyFrequencies)
		{
			float frequency;
			bodyFrequencyVar >> frequency;
			size_t index = mProceduralUnitFactory->GetBody(bodyFrequencyVar.GetName())->mIndexInFactory;

			cumulativeBodyFrequencies.emplace_back(index, frequency + totalFrequency);
			totalFrequency += frequency;
		}

		const std::vector<Framework::Data::Variable>& turretFrequencies = group.GetScope("turrets").GetVariables();
		std::vector<ProceduralUnitFactory::CumulativeFrequency> cumulativeTurretFrequencies{};
		totalFrequency = 0.0f;
		for (const Framework::Data::Variable& turretFrequencyVar : turretFrequencies)
		{
			float frequency;
			turretFrequencyVar >> frequency;
			size_t index = mProceduralUnitFactory->GetTurret(turretFrequencyVar.GetName())->mIndexInFactory;

			cumulativeTurretFrequencies.emplace_back(index, frequency + totalFrequency);
			totalFrequency += frequency;
		}

		std::vector<Unit*> units(amount);
		for (uint i = 0; i < amount; i++)
		{
			units[i] = &AddToArmy<Unit>();
			mProceduralUnitFactory->AssignRandomParts(*units[i], *mScene.mEntityManager, cumulativeBodyFrequencies, cumulativeTurretFrequencies);
		}

		glm::vec2 centre;
		group.GetVariable("centre") >> centre;

		std::vector<glm::vec2> formation = GenerateFormation(units, { centre });

		for (uint i = 0; i < amount; i++)
		{
			const glm::vec2& pos = formation[i];
			units[i]->ForceSetPosition(pos);
		}
	}
}

const std::vector<Framework::EntityId>& RTS::Army::GetIds()
{
	RemoveInvalidIds();
	return mUnits;
}

std::vector<RTS::Unit*> RTS::Army::GetUnitsInArmy()
{
	RemoveInvalidIds();

	const Framework::EntityManager* const entityManager = mScene.mEntityManager.get();

	std::vector<Unit*> units(mUnits.size());

	for (uint i = 0; i < mUnits.size(); i++)
	{
		units[i] = static_cast<Unit*>(entityManager->GetEntity(mUnits[i]));
	}

	return units;
}

bool RTS::Army::Serialize(Framework::Data::Scope& parentScope) const
{
	Entity::Serialize(parentScope);
	Framework::Data::Scope& myScope = parentScope.AddChild("Army");

	myScope.AddVariable("units") << mUnits;
	myScope.AddVariable("armyId") << static_cast<uchar>(mArmyId);
	myScope.AddVariable("numOfUnitsSpawned") << mNumOfUnitsSpawned;


	return true;
}

void RTS::Army::Deserialize(const Framework::Data::Scope& parentScope)
{
	Entity::Deserialize(parentScope);

	const Framework::Data::Scope& myScope = parentScope.GetScope("Army");
	myScope.GetVariable("units") >> mUnits;
	uchar tmp;
	myScope.GetVariable("armyId") >> tmp;
	mArmyId = static_cast<ArmyId>(tmp);
	myScope.GetVariable("numOfUnitsSpawned") >> mNumOfUnitsSpawned;
}

size_t RTS::Army::Size()
{
	RemoveInvalidIds();
	return mUnits.size();
}

void RTS::Army::RemoveInvalidIds()
{
	Framework::EntityManager* entityManager = mScene.mEntityManager.get();

	mUnits.erase(remove_if(mUnits.begin(), mUnits.end(), [entityManager](const Framework::EntityId& id)
		{
			return !entityManager->TryGetEntity(id).has_value();
		}), mUnits.end());
}
