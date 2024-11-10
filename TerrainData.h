#pragma once
#include "BoundingBox2D.h"

namespace Framework
{
	class TerrainData
	{
	public:
		TerrainData(const uint numOfChunksX, const uint numOfChunksZ);

		// Returns the percentage of the heightmap that has been generated after this function completes.
		virtual float GenerateHeightMap(const size_t maxNumOfSamples = std::numeric_limits<size_t>::max());

		inline const std::vector<float>& GetHeightMap() const { return mHeightMap; }
		inline const std::vector<glm::vec3>& GetNormals() const { return mNormals; }

		float GetHeightAtPositionFast(const float x, const float z) const;

		float GetHeightAtPosition(const float x, const float z) const;
		glm::vec3 GetNormalAtPosition(const float x, const float z) const;

		float GetHeightAtIndex(const uint index) const;
		glm::vec3 GetNormalAtPosition(const uint index) const;

		glm::uvec2 WorldToSample(const float x, const float z) const;

		float GetHeighestVertexHeight() const { return mHeightestVertexHeight; }

		template<typename T>
		inline T BilinearInterpolation(const std::vector<T>& data, const float x, const float z) const;

		inline btBoxShape* GetChunkBoxShape() const { return mChunkBoxShape.get();}
	private:
		std::unique_ptr<btBoxShape> mChunkBoxShape{};
	public:

		const uint mNumOfChunksX{};
		const uint mNumOfChunksZ{};

		const float mWorldSizeX{};
		const float mWorldSizeZ{};
		
		const uint mNumOfVerticesX{};
		const uint mNumOfVerticesZ{};

		const BoundingBox2D mWorldBounds{};

		void SetHeightMap(std::vector<float> heightMap);

	private:
		void RecalculateNormals();

		std::vector<glm::vec3> mNormals{};
		std::vector<float> mHeightMap{};

		float mHeightestVertexHeight{};
	};
}