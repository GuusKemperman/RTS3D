#pragma once
#include "TerrainData.h"
#include "Chunk.h"

class btHeightfieldTerrainShape;

namespace Framework
{
	namespace Data
	{
		class Scope;
	}

	class Scene;

	class Terrain
	{
	public:
		Terrain(Scene& scene);
		~Terrain();

		inline TerrainData* GetData() const { return mData.get(); }
		
		inline float GetHeightAtPosition(const float x, const float z) const { return mData->GetHeightAtPosition(x, z); }
		inline glm::vec3 GetNormalAtPosition(const float x, const float z) const { return mData->GetNormalAtPosition(x, z); }

		inline float GetWorldSizeX() const { return mData->mWorldSizeX; }
		inline float GetWorldSizeZ() const { return mData->mWorldSizeZ; }

		void SetTerrainData(std::unique_ptr<TerrainData> data);

		void GenerateNoiseForNonRepeatTexture(const uint seed);

		// The terrain can generate chunks in parts.
		// Returns the percentage of chunks that have been generated.
		float GenerateTerrain(const uint maxNumOfChunksToGenerate = std::numeric_limits<uint>::max());
		
		void Serialize(Framework::Data::Scope& parentScope) const;
		void Deserialize(const Framework::Data::Scope& parentScope);

		void OnSettingsChange(const Data::Scope& previousSettings, const Data::Scope& currentSettings);

	private:
		void SendTerrainToPhysics();

		std::unique_ptr<btHeightfieldTerrainShape> mCollisionShape{};
		std::unique_ptr<btCollisionObject> mCollisionObject{};
		Scene& mScene;

		std::unique_ptr<TerrainData> mData{};

		uint mNumOfChunksGenerated{};
		uint mSeedForNonRepeatTexture{};
	};
}