#include "precomp.h"
#include "ProceduralUnitFactory.h"

#include <filesystem>

#include "SavedData.h"
#include "AssetManager.h"
#include "Mesh.h"
#include "Unit.h"
#include "Frustum.h"
#include "Turret.h"
#include "EntityManager.h"

RTS::ProceduralUnitFactory::ProceduralUnitFactory(const std::string& directory)
{
	const std::filesystem::path bodyPath = directory + "/bodies";
	for (const std::filesystem::directory_entry& dirEntry : std::filesystem::directory_iterator{ bodyPath })
	{
		UnitBodyData data = { dirEntry.path().string().substr(sDataRoot.size()) };
		data.mIndexInFactory = mBodyData.size();
		mBodyData.push_back(std::move(data));
	}
	mBodyData.shrink_to_fit();

	const std::filesystem::path turretPath = directory + "/turrets";
	for (const std::filesystem::directory_entry& dirEntry : std::filesystem::directory_iterator{ turretPath })
	{
		TurretData data = { dirEntry.path().string().substr(sDataRoot.size()) };
		data.mIndexInFactory = mTurretData.size();
		mTurretData.push_back(std::move(data));
	}
	mTurretData.shrink_to_fit();
}

size_t RTS::ProceduralUnitFactory::RandomIndex(const std::vector<CumulativeFrequency>& frequencies)
{
	const float randomNumber = Framework::Random::Range(1.0f);

	size_t bestIndex = 0;

	for (size_t i = 1; i < frequencies.size(); i++)
	{
		const float value = frequencies[i].second > randomNumber ? frequencies[i].second : INFINITY;
		const float bestValue = frequencies[bestIndex].second> randomNumber ? frequencies[bestIndex].second : INFINITY;

		if (value < bestValue)
		{
			bestIndex = i;
		}
	}

	return bestIndex;
}

void RTS::ProceduralUnitFactory::AssignRandomParts(Unit& unit, Framework::EntityManager& whatToAddTheTurretsTo, 
	const std::vector<CumulativeFrequency>& bodyCumulaitiveFrequencies, const std::vector<CumulativeFrequency>& turretCumulaitiveFrequencies) const
{
	const UnitBodyData* body = &mBodyData.at(RandomIndex(bodyCumulaitiveFrequencies));
	unit.SetUnitBodyData(body);

	if (body->mPossibleTurretNodes.empty())
	{
		return;
	}

	const size_t numOfTurrets = Framework::Random::Range(1, static_cast<uint>(body->mPossibleTurretNodes.size()));
	std::vector<size_t> nodeIndices(body->mPossibleTurretNodes.size());
	for (size_t i = 0; i < body->mPossibleTurretNodes.size(); i++)
	{
		nodeIndices[i] = i;
	}

	for (size_t i = 0; i < numOfTurrets; i++)
	{
		size_t randomIndexInNodeIndices = rand() % nodeIndices.size();
		size_t randomNodeIndex = nodeIndices[randomIndexInNodeIndices];

		// Remove it so we dont place a turret twice on the same node
		nodeIndices[randomIndexInNodeIndices] = nodeIndices.back();
		nodeIndices.pop_back();

		const Framework::Transform& node = body->mPossibleTurretNodes[randomNodeIndex];

		const TurretData* turret = &mTurretData.at(RandomIndex(turretCumulaitiveFrequencies));
		whatToAddTheTurretsTo.AddEntity<Turret>(turret, &unit, node);
	}
}

const RTS::UnitBodyData* RTS::ProceduralUnitFactory::GetBody(const std::string& name) const
{
	auto it = std::find_if(mBodyData.begin(), mBodyData.end(),
		[name](const UnitBodyData& bodyData)
		{
			return bodyData.mName == name;
		});

	assert(it != mBodyData.end());
	return &*it;
}

const RTS::TurretData* RTS::ProceduralUnitFactory::GetTurret(const std::string& name) const
{
	auto it = std::find_if(mTurretData.begin(), mTurretData.end(),
		[name](const TurretData& turretData)
		{
			return turretData.mName == name;
		});

	assert(it != mTurretData.end());
	return &*it;
}

RTS::UnitBodyData::UnitBodyData(const std::string& filePath)
{
	Framework::Data::SavedData savedData = { filePath };

	std::string meshPathArmy1;
	savedData.GetVariable("meshPathArmy1") >> meshPathArmy1;
	mMeshesForEachArmy[0] = Framework::AssetManager::Inst().GetAsset<Framework::Mesh>(meshPathArmy1)->GetMeshId();

	std::string meshPathArmy2;
	savedData.GetVariable("meshPathArmy2") >> meshPathArmy2;
	mMeshesForEachArmy[1] = Framework::AssetManager::Inst().GetAsset<Framework::Mesh>(meshPathArmy2)->GetMeshId();

	Framework::Transform colliderTransform{};


	glm::vec3 halfExtends;
	savedData.GetVariable("halfExtends") >> halfExtends;
	mShape = std::make_unique<btBoxShape>(Framework::Math::ToBullet(halfExtends));
	

	Framework::Data::Scope turretNodeScope = savedData.GetScope("turretNodes");
	for (const Framework::Data::Scope& child : turretNodeScope.GetChildren())
	{
		Framework::Transform transform{};
		transform.LoadFrom(child);

		mPossibleTurretNodes.push_back(std::move(transform));
	}

	savedData.GetVariable("name") >> mName;
	savedData.GetVariable("hoverAtHeight") >> mHoverAtHeight;
	savedData.GetVariable("maxHealth") >> mMaxHealth;
	savedData.GetVariable("movementSpeed") >> mMovementSpeed;
	savedData.GetVariable("turnSpeed") >> mTurnSpeed;
	savedData.GetVariable("mass") >> mMass;
	savedData.GetVariable("deathExplosionSize") >> mDeathExplosionSize;
;


	mShape->calculateLocalInertia(mMass, mInertia);
}

RTS::TurretData::TurretData(const std::string& filePath)
{
	const Framework::Data::SavedData savedData = { filePath };

	float width, height, fov, zNear, zFar;
	const Framework::Data::Scope& shapeScope = savedData.GetScope("canFireWithinShape");
	shapeScope.GetVariable("width") >> width;
	shapeScope.GetVariable("height") >> height;
	shapeScope.GetVariable("fov") >> fov;
	shapeScope.GetVariable("zNear") >> zNear;
	shapeScope.GetVariable("zFar") >> zFar;

	mCanFireWithinShape = std::make_unique<Framework::FrustumShape>(width, height, fov, zNear, zFar);

	std::string meshPath;
	savedData.GetVariable("meshPath") >> meshPath;
	mMeshId = Framework::AssetManager::Inst().GetAsset<Framework::Mesh>(meshPath)->GetMeshId();

	savedData.GetVariable("name") >> mName;
	savedData.GetVariable("cooldown") >> mCooldown;
	savedData.GetVariable("turnSpeed") >> mTurnSpeed;
	savedData.GetVariable("damage") >> mDamage;
}
