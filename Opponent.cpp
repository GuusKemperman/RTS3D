#include "precomp.h"
#include "Opponent.h"

#include "Army.h"
#include "Commands.h"
#include "Unit.h"
#include "Scene.h"
#include "Terrain.h"
#include "TerrainData.h"
#include "Scope.h"

RTS::Opponent::Opponent(Framework::Scene& scene, Framework::EntityId armyEntityId) :
	Entity(scene)
{
	mHasFixedTick = true;

	mArmy = dynamic_cast<Army*>(scene.mEntityManager->TryGetEntity(armyEntityId).value_or(nullptr));
}

void RTS::Opponent::FixedTick()
{
	std::vector<Unit*> units = mArmy->GetUnitsInArmy();

	std::vector<Unit*> unitsToCommand;
	unitsToCommand.reserve(units.size());

	for (Unit* unit : units)
	{
		unit->SetAggroLevel(AggroLevel::pursuit);

		if (Framework::Random::Range(1.0f) <= sGiveCommandChance)
		{
			unitsToCommand.push_back(unit);
		}
	}

	for (uint i = 0; i < unitsToCommand.size(); i++)
	{
		const uint desiredGroupSize = RandomGroupSize();

		std::vector<Unit*> group = { unitsToCommand[i] };
		group.reserve(desiredGroupSize);

		for (uint j = i + 1; j < unitsToCommand.size() && group.size() < desiredGroupSize; j++, i++)
		{
			group.push_back(unitsToCommand[j]);
		}

		OrderToRandomPosition(std::move(group));
	}
}

bool RTS::Opponent::Serialize(Framework::Data::Scope& parentScope) const
{
	Entity::Serialize(parentScope);

	Framework::Data::Scope& myScope = parentScope.AddChild("Opponent");
	myScope.AddVariable("armyEntityId") << mArmy->GetId();

	return true;
}

void RTS::Opponent::Deserialize(const Framework::Data::Scope& parentScope)
{
	Entity::Deserialize(parentScope);

	const Framework::Data::Scope& myScope = parentScope.GetScope("Opponent");

	Framework::EntityId id;
	myScope.GetVariable("armyEntityId") >> id;
	mScene.mEntityManager->DelayedIdRequest(id,
		[](Entity& entity, void* me)
		{
			static_cast<Opponent*>(me)->SetArmy(static_cast<Army*>(&entity));
		}, this);
}

void RTS::Opponent::OrderToRandomPosition(std::vector<Unit*>&& group) const
{
	const Framework::TerrainData* const terrainData = mScene.mTerrain->GetData();
	glm::vec2 position = { Framework::Random::Range(sMinDistFromEdge, terrainData->mWorldSizeX - sMinDistFromEdge), Framework::Random::Range(sMinDistFromEdge, terrainData->mWorldSizeZ - sMinDistFromEdge) };

	FormFormation(std::move(group), position);
}

uint RTS::Opponent::RandomGroupSize() const
{
	return Framework::Random::Uint() % (sMaxGroupSize - sMinGroupSize) + sMinGroupSize;
}