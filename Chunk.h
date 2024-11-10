#pragma once
#include "Entity.h"

namespace Framework
{
	class Terrain;
	class Mesh;
	class Camera;

	class Chunk :
		public Entity
	{
		ENTITYMAKER(Chunk);
	public:
		Chunk(Scene& scene, const glm::vec2 position = {0.0f, 0.0f});
		~Chunk();

		void Draw() const override;

		// Chunk's aren't serialized since they can be more easily generated with the terrain serialization.
		bool Serialize(Data::Scope&) const override { return false; };
		void Deserialize(const Data::Scope&) override {};

		static constexpr uint sSizeX = 130u;
		static constexpr uint sSizeZ = 130u;

		static constexpr float sVertexResolution = 0.5f;
		static constexpr float sTextureResolution = 4.0f;
		static constexpr float sSpaceBetweenVertices = 1.0f / static_cast<float>(sVertexResolution);

		static constexpr ushort sNumOfVerticesX = static_cast<ushort>(static_cast<float>(sSizeX) / sSpaceBetweenVertices) + 1;
		static constexpr ushort sNumOfVerticesZ = static_cast<ushort>(static_cast<float>(sSizeZ) / sSpaceBetweenVertices) + 1;
	private:
		void GenerateMesh(const glm::vec3 position, const Terrain& terrain);

		glm::mat4 mModelMatrix{};
		std::unique_ptr<Mesh> mMesh{};

		uint mFrameLastInsideFrustum{};
	};
}