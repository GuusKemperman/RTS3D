#include "precomp.h"
#include "Hills.h"

#include "Chunk.h"

RTS::Hills::Hills(const uint numOfChunksX, const uint numOfChunksZ, const float maxHeight, const float roughness, const float persistence, const uint octaves) :
	TerrainData(numOfChunksX, numOfChunksZ),
	mMaxHeight(maxHeight),
	mRoughness(roughness),
	mPersistence(persistence),
	mOctaves(octaves)
{
	mPerlin.reseed(Framework::Random::Uint());
	mIncompleteHeightmap.reserve(mNumOfVerticesX * mNumOfVerticesZ);
}

float RTS::Hills::GenerateHeightMap(const size_t maxNumOfSamples)
{
	size_t vertexIndex = mIncompleteHeightmap.size();
	size_t samplesTaken{};

	for (size_t z = vertexIndex / mNumOfVerticesX; z < mNumOfVerticesZ && samplesTaken < maxNumOfSamples; z++)
	{
		for (size_t x = vertexIndex % mNumOfVerticesX; x < mNumOfVerticesX && samplesTaken < maxNumOfSamples; x++, vertexIndex++, samplesTaken++)
		{
			const float noise = mPerlin.octave2D_01(static_cast<float>(x) * mRoughness, static_cast<float>(z) * mRoughness, mOctaves, mPersistence);
			mIncompleteHeightmap.push_back(noise * mMaxHeight);
		}
	}

	const size_t totalAmountOfVertices = static_cast<size_t>(mNumOfVerticesX) * static_cast<size_t>(mNumOfVerticesZ);
	const size_t amountLeft = totalAmountOfVertices - vertexIndex;

	if (amountLeft == 0)
	{
		SetHeightMap(std::move(mIncompleteHeightmap));
	}

	return static_cast<float>(vertexIndex) / static_cast<float>(totalAmountOfVertices);
}
