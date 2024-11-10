#include "precomp.h"
#include "TerrainData.h"

#include "Chunk.h"

Framework::TerrainData::TerrainData(const uint numOfChunksX, const uint numOfChunksZ) :
	mNumOfChunksX(numOfChunksX),
	mNumOfChunksZ((numOfChunksZ)),
	mWorldSizeX(static_cast<float>(Chunk::sSizeX) * static_cast<float>(numOfChunksX)),
	mWorldSizeZ(static_cast<float>(Chunk::sSizeZ) * static_cast<float>(numOfChunksZ)),
	mNumOfVerticesX(static_cast<uint>(static_cast<float>(numOfChunksX * Chunk::sSizeX) / Chunk::sSpaceBetweenVertices) + 1),
	mNumOfVerticesZ(static_cast<uint>(static_cast<float>(numOfChunksZ * Chunk::sSizeZ) / Chunk::sSpaceBetweenVertices) + 1),
	mWorldBounds({0.0f, 0.0f, mWorldSizeX, mWorldSizeZ})
{
}

float Framework::TerrainData::GenerateHeightMap(const size_t)
{
	SetHeightMap(std::vector<float>(mNumOfVerticesX * mNumOfVerticesZ));
	return 1.0f;
}

template<typename T>
inline T Framework::TerrainData::BilinearInterpolation(const std::vector<T>& data, const float x, const float z) const
{
	const glm::uvec2 sampleLocation = WorldToSample(x, z);
	const uint sampleIndex = sampleLocation.x + sampleLocation.y * mNumOfVerticesX;

	const bool onEdgeX = sampleLocation.x + 1u >= mNumOfVerticesX;
	const bool onEdgeZ = sampleLocation.y + 1u >= mNumOfVerticesZ;

	const T sampleX0Z0 = data[sampleIndex];
	const T sampleX1Z0 = onEdgeX ? sampleX0Z0 : data[sampleIndex + 1];
	const T sampleX0Z1 = onEdgeZ ? sampleX0Z0 : data[sampleIndex + mNumOfVerticesX];
	const T sampleX1Z1 = onEdgeX || onEdgeZ ? sampleX0Z0 : data[sampleIndex + mNumOfVerticesX + 1];

	const float weightX = fmodf(x, Chunk::sSpaceBetweenVertices);
	const float weightZ = fmodf(z, Chunk::sSpaceBetweenVertices);

	const float opposingWeightX = Chunk::sSpaceBetweenVertices - weightX;
	const float opposingWeightY = Chunk::sSpaceBetweenVertices - weightZ;
	constexpr float factor = 1.0f / (Chunk::sSpaceBetweenVertices * Chunk::sSpaceBetweenVertices);

	return factor * (
		sampleX0Z0 * opposingWeightX * opposingWeightY +
		sampleX1Z0 * weightX * opposingWeightY +
		sampleX0Z1 * opposingWeightX * weightZ +
		sampleX1Z1 * weightX * weightZ
		);
}

float Framework::TerrainData::GetHeightAtPositionFast(const float x, const float z) const
{
	const glm::uvec2 sampleLocation = WorldToSample(x, z);
	return mHeightMap[sampleLocation.x + sampleLocation.y * mNumOfVerticesX];
}

float Framework::TerrainData::GetHeightAtPosition(const float x, const float z) const
{
	return BilinearInterpolation<float>(mHeightMap, x, z);
}

glm::vec3 Framework::TerrainData::GetNormalAtPosition(const float x, const float z) const
{
	return BilinearInterpolation<glm::vec3>(mNormals, x, z);
}

float Framework::TerrainData::GetHeightAtIndex(const uint index) const
{
	assert(index >= 0
		&& index < mNumOfVerticesX * mNumOfVerticesZ);
	return mHeightMap[index];
}

glm::vec3 Framework::TerrainData::GetNormalAtPosition(const uint index) const
{
	assert(index >= 0
		&& index < mNumOfVerticesX* mNumOfVerticesZ);
	return mNormals[index];
}

glm::uvec2 Framework::TerrainData::WorldToSample(const float x, const float z) const
{
	const uint sampleX = static_cast<uint>(glm::clamp(x, 0.0f, mWorldSizeX) / Chunk::sSpaceBetweenVertices);
	const uint sampleZ = static_cast<uint>(glm::clamp(z, 0.0f, mWorldSizeZ) / Chunk::sSpaceBetweenVertices);

	return { sampleX, sampleZ };
}

void Framework::TerrainData::SetHeightMap(std::vector<float> heightMap)
{
	assert(heightMap.size() == mNumOfVerticesX * mNumOfVerticesZ);
	mHeightMap = std::move(heightMap);

	if (mHeightMap.empty())
	{
		return;
	}

	// Normalize the heights so that the lowest is at height 0.0f
	const float lowestHeight = *std::min_element(mHeightMap.begin(), mHeightMap.end());
	
	for (float& height : mHeightMap)
	{
		height -= lowestHeight;
	}

	mHeightestVertexHeight = *std::max_element(mHeightMap.begin(), mHeightMap.end());

	btVector3 halfExtends = { Chunk::sSizeX * 0.5f, mHeightestVertexHeight * .5f, Chunk::sSizeZ * 0.5f };
	mChunkBoxShape = std::make_unique<btBoxShape>(halfExtends);

	RecalculateNormals();
}

void Framework::TerrainData::RecalculateNormals()
{
	const size_t numOfVertices = mNumOfVerticesX * mNumOfVerticesZ;

	assert(numOfVertices == mHeightMap.size());

	mNormals = std::vector(numOfVertices, glm::vec3{ 0.0f, 1.0f, 0.0f });

	for (uint z = 1u; z < mNumOfVerticesZ - 1u; z++)
	{
		for (uint x = 1u; x < mNumOfVerticesX - 1u; x++)
		{
			const uint vertexIndex = x + z * mNumOfVerticesX;

			constexpr float eps = Chunk::sSpaceBetweenVertices;

			const float fx0 = mHeightMap[vertexIndex - 1];
			const float fx1 = mHeightMap[vertexIndex + 1];
			const float fy0 = mHeightMap[vertexIndex - mNumOfVerticesX];
			const float fy1 = mHeightMap[vertexIndex + mNumOfVerticesX];

			mNormals[vertexIndex] = normalize(glm::vec3((fx0 - fx1) / (2 * eps), 1, (fy0 - fy1) / (2 * eps)));
		}
	}
}