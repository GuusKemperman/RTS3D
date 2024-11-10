#include "precomp.h"
#include "Forest.h"

#include <chrono>

#include "Tree.h"
#include "EntityManager.h"
#include "Scene.h"
#include "Terrain.h"
#include "TerrainData.h"

RTS::Forest::Forest(Framework::Scene& scene, const float forestSize, const float worldCoverage, const float maxSteepness) :
	mScene(scene),
	mForestSize(forestSize),
	mWorldCoverage(worldCoverage),
	mMaxSteepness(maxSteepness)
{
}

float RTS::Forest::SpawnTrees(const uint maxAmountToSpawnThisCycle)
{
	if (!mTreeLocations.has_value())
	{
		return 0.0f;
	}

	siv::BasicPerlinNoise<float> perlin;
	perlin.reseed(Framework::Random::Uint());

	const Framework::TerrainData* const terrainData = mScene.mTerrain->GetData();
	Framework::EntityManager* const entityManager = mScene.mEntityManager.get(); 

	uint numOfPoints = static_cast<uint>(mTreeLocations.value().size());

	for (uint spawnedThisCycle = 0; mPointIndex < numOfPoints && spawnedThisCycle < maxAmountToSpawnThisCycle; mPointIndex++, spawnedThisCycle++)
	{
		const PoissonGenerator::Point& point = mTreeLocations.value()[mPointIndex];

		const glm::vec2 obstaclePosition = { point.x * terrainData->mWorldSizeX, point.y * terrainData->mWorldSizeZ };

		const float noise = perlin.noise2D_01(obstaclePosition.x / mForestSize, obstaclePosition.y / mForestSize);

		if (noise > mWorldCoverage)
		{
			continue;
		}

		const float steepness = 1.0f - glm::dot(glm::vec3{ 0.0f, 1.0f, 0.0f }, terrainData->GetNormalAtPosition(obstaclePosition.x, obstaclePosition.y));

		if (steepness <= mMaxSteepness)
		{
			Framework::Transform& obstacleTransform = entityManager->AddEntity<RTS::Tree>(obstaclePosition).GetTransform();

			glm::vec3 obstacleScale = obstacleTransform.GetLocalScale();
			//obstacleScale.y *= 1.0f + noise;

			obstacleTransform.SetLocalScale(obstacleScale);
		}
	}

	return static_cast<float>(mPointIndex) / static_cast<float>(numOfPoints);
}

void RTS::Forest::StartCalculatingLocations()
{	
	const Framework::TerrainData* terrainData = mScene.mTerrain->GetData();
	PoissonGenerator::DefaultPRNG PRNG = PoissonGenerator::DefaultPRNG{ Framework::Random::Uint() };

	const float spacing = std::max(RTS::Tree::sMinDistBetweenTrees / terrainData->mWorldSizeX, RTS::Tree::sMinDistBetweenTrees / terrainData->mWorldSizeZ);
	const float treeArea = PI * (spacing * spacing);
	const uint numOfTrees = static_cast<uint>(0.785f / treeArea) * 2u;

	mTreeLocations = PoissonGenerator::generatePoissonPoints(numOfTrees, PRNG, false, 5u, spacing);
}