#include "precomp.h"
#include "Chunk.h"

#include "AssetManager.h"
#include "Terrain.h"
#include "Mesh.h"
#include "MyShader.h"
#include "Material.h"
#include "Scene.h"
#include "Camera.h"
#include "Transform.h"
#include "Physics.h"
#include "Scope.h"

Framework::Chunk::Chunk(Scene& scene, const glm::vec2 position) : 
	Entity(scene)
{
	Transform& transform = GetTransform();
	transform.SetLocalPosition(position.x, scene.mTerrain->GetData()->GetHeighestVertexHeight() * .5f, position.y);
	mModelMatrix = transform.GetLocalMatrix();

	GenerateMesh(transform.GetLocalPosition(), *scene.mTerrain);

	mCollisionObject = std::make_unique<btCollisionObject>();

	mCollisionObject->setCollisionShape(scene.mTerrain->GetData()->GetChunkBoxShape());
	mCollisionObject->setUserPointer(this);
	mCollisionObject->setWorldTransform(transform.ToBullet());
	mCollisionObject->setCollisionFlags(mCollisionObject->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

	mScene.mPhysics->AddCollisionObjectToWorld(mCollisionObject.get(), Physics::Group::visibileButNoCollisionGroup, Physics::Mask::visibleButNoCollisionMask);
}

Framework::Chunk::~Chunk()
{
	mScene.mPhysics->RemoveCollisionObjectFromWorld(std::move(mCollisionObject));
}

void Framework::Chunk::Draw() const
{
	const MyShader* shader = mMesh->GetShader();
	shader->Bind();

	shader->SetInputTexture(2, "flatSampler", *mMesh->GetMaterial()->GetDiffuse());
	shader->SetInputTexture(3, "steepSampler", *mMesh->GetMaterial()->GetAlpha());
	shader->SetInputMatrix("MVP", mScene.mCamera->GetViewProjection() * mModelMatrix);
	shader->SetInputMatrix("modelMatrix", mModelMatrix);
	shader->SetFloat3("cameraPos", mScene.mCamera->GetTransform().GetLocalPosition());
	mMesh->SimpleDraw();

	shader->Unbind();
}

void Framework::Chunk::GenerateMesh(const glm::vec3 position, const Terrain& terrain)
{
	static_assert(static_cast<unsigned long>(sNumOfVerticesX) * static_cast<unsigned long>(sNumOfVerticesZ) < std::numeric_limits<ushort>::max());

	const ushort numOfVertices = sNumOfVerticesX * sNumOfVerticesZ;

	std::vector<glm::vec3> vertices(numOfVertices);
	std::vector<glm::vec3> normals(numOfVertices);

	std::vector<glm::vec2> UVs(numOfVertices);

	const TerrainData* const terrainData = terrain.GetData();

	const glm::vec2 chunkCorner = glm::vec2{ position.x, position.z } - glm::vec2{ sSizeX, sSizeZ } * 0.5f;
	const glm::ivec2 sampleStart = terrainData->WorldToSample(chunkCorner.x, chunkCorner.y);
	uint sampleIndex = sampleStart.x + sampleStart.y * terrainData->mNumOfVerticesX;

	// Generate vertices
	for (ushort z = 0; z < sNumOfVerticesZ; z++, sampleIndex += terrainData->mNumOfVerticesX)
	{
		for (ushort x = 0; x < sNumOfVerticesX; x++)
		{
			// Weird work around to surpress a compiler warning.
			const ushort vertexIndex = static_cast<ushort>(static_cast<int>(x) + static_cast<int>(z) * static_cast<int>(sNumOfVerticesX));
			
			glm::vec3& localVertexPosition = vertices[vertexIndex];
			localVertexPosition = { static_cast<float>(x) * sSpaceBetweenVertices - sSizeX * 0.5f, terrainData->GetHeightAtIndex(sampleIndex + x) - position.y, static_cast<float>(z) * sSpaceBetweenVertices - sSizeZ * 0.5f};

			UVs[vertexIndex] = { (localVertexPosition.x * sTextureResolution) / sSizeX, (localVertexPosition.z * sTextureResolution) / sSizeZ};
			normals[vertexIndex] = terrainData->GetNormalAtPosition(sampleIndex + x);
		}
	}

	// Generate triangles
	std::vector<Mesh::Triangle> triangles((sNumOfVerticesX - 1) * (sNumOfVerticesZ - 1) * 2);

	for (ushort z = 0, triangleIndex = 0; z < sNumOfVerticesZ - 1; z++)
	{
		for (ushort x = 0; x < sNumOfVerticesX - 1; x++, triangleIndex += 2)
		{
			// Weird work around to surpress a compiler warning.
			const ushort vertexIndex = static_cast<ushort>(static_cast<int>(x) + static_cast<int>(z) * static_cast<int>(sNumOfVerticesX));

			triangles[triangleIndex] = { vertexIndex, static_cast<ushort>(vertexIndex + 1 + sNumOfVerticesX), static_cast<ushort>(vertexIndex + 1) };
			triangles[triangleIndex + 1] = { vertexIndex, static_cast<ushort>(vertexIndex + sNumOfVerticesX),  static_cast<ushort>(vertexIndex + 1 + sNumOfVerticesX) };
		}
	}

	mMesh = std::make_unique<Mesh>();

	mMesh->SetTriangles(std::move(triangles));
	mMesh->SetVertices(std::move(vertices));
	mMesh->SetNormals(std::move(normals));
	mMesh->SetUVs(std::move(UVs));

	AssetManager& assetManager = AssetManager::Inst();
	
	mMesh->SetShader(assetManager.GetAsset<MyShader>("shaders/terrain.vert,shaders/terrain.frag"));
	mMesh->SetMaterial(assetManager.GetAsset<Material>("materials/terrain.mtl"));
}