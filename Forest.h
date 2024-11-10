#pragma once

namespace Framework
{
	class Scene;
	class TerrainData;
}

namespace RTS
{
	class Forest
	{
	public:
		Forest(Framework::Scene& scene, const float forestSize, const float worldCoverage, const float maxSteepness);

		void StartCalculatingLocations();

		float SpawnTrees(const uint maxAmountToSpawnThisCycle = std::numeric_limits<uint>::max());

	private:

		Framework::Scene& mScene;

		const float mForestSize{};
		const float mWorldCoverage{};
		const float mMaxSteepness{};

		uint mPointIndex = 0;

		std::optional<std::vector<PoissonGenerator::Point>> mTreeLocations{};
	};
}