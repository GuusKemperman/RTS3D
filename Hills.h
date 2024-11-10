#pragma once
#include "TerrainData.h"

namespace RTS
{
    class Hills :
        public Framework::TerrainData
    {
    public:
        Hills(const uint numOfChunksX, const uint numOfChunksZ, const float maxHeight, const float roughness, const float persistence, const uint octaves);

        // Returns the percentage of the heightmap that has been generated after this function completes.
        float GenerateHeightMap(const size_t maxNumOfSamples = std::numeric_limits<size_t>::max()) override;

    private:
        std::vector<float> mIncompleteHeightmap{};
        siv::BasicPerlinNoise<float> mPerlin{};

        const float mMaxHeight{};
        const float mRoughness{};
        const float mPersistence{};
        const uint mOctaves{};
    };
}