#include "precomp.h"
#include "Terrain.h"

#include "Scene.h"
#include "EntityManager.h"
#include "Chunk.h"
#include "MyShader.h"
#include "Material.h"
#include "Mesh.h"
#include "Physics.h"
#include "Transform.h"
#include "Camera.h"
#include "Scope.h"
#include "AssetManager.h"
#include "Texture.h"
#include "Surface.h"
#include "Settings.h"

Framework::Terrain::Terrain(Scene& scene) :
	mScene(scene)
{
	Settings::Inst().mOnSettingsChanged.bind(this, &Terrain::OnSettingsChange);
}

Framework::Terrain::~Terrain()
{
	if (mCollisionObject != nullptr)
	{
		mScene.mPhysics->RemoveCollisionObjectFromWorld(std::move(mCollisionObject));
	}

	Settings::Inst().mOnSettingsChanged.unbind(this, &Terrain::OnSettingsChange);
}

void Framework::Terrain::SetTerrainData(std::unique_ptr<TerrainData> data)
{
	if (mData != nullptr)
	{
		LOGWARNING("Terrain data has already been set before. Make sure to regenerate the terrain.");
	}

	mData = std::move(data);
}

void Framework::Terrain::GenerateNoiseForNonRepeatTexture(const uint seed)
{
	// Let's store the seed we used to generate this noise, so we can serialize it.
	mSeedForNonRepeatTexture = seed;
	uint currentSeed = seed;

	// We can store the noise in the alpha to prevent sending an additional texture
	Texture* diffuse = AssetManager::Inst().GetAsset<Material>("materials/terrain.mtl")->GetDiffuse();
	Surface* surface = diffuse->GetSurface();

	const size_t numOfPixels = surface->mWidth * surface->mHeight;
	for (size_t i = 0; i < numOfPixels; i++)
	{
		uint& pixel = surface->mPixels[i];
		uint randomNum = Random::Uint(currentSeed) % 255;

		pixel = (pixel & 0xffffff) + (randomNum << 24);
	}

	diffuse->SyncSurfaceAndTexture();
}

float Framework::Terrain::GenerateTerrain(const uint maxNumOfChunksToGenerate)
{
	assert(mData != nullptr);

	const uint numOfChunksToMake = mData->mNumOfChunksX * mData->mNumOfChunksZ;

	uint chunkIndex = mNumOfChunksGenerated;
	uint createdThisCycle = 0;

	for (uint z = chunkIndex / mData->mNumOfChunksX; z < mData->mNumOfChunksZ && createdThisCycle < maxNumOfChunksToGenerate; z++)
	{
		for (uint x = chunkIndex % mData->mNumOfChunksX; x < mData->mNumOfChunksX && createdThisCycle < maxNumOfChunksToGenerate; x++, chunkIndex++, createdThisCycle++)
		{
			glm::vec2 chunkPosition = { static_cast<float>(static_cast<float>(x) * Chunk::sSizeX),
				static_cast<float>(static_cast<float>(z) * Chunk::sSizeZ) };
			chunkPosition += glm::vec2{ Chunk::sSizeX, Chunk::sSizeZ } *0.5f;

			mScene.mEntityManager->AddEntity<Chunk>(chunkPosition);
		}
	}

	mNumOfChunksGenerated += createdThisCycle;

	const uint amountLeft = numOfChunksToMake - chunkIndex;
	if (amountLeft == 0)
	{
		SendTerrainToPhysics();
	}

	const float percentageGenerated = static_cast<float>(chunkIndex) / static_cast<float>(numOfChunksToMake);
	return percentageGenerated;
}

void Framework::Terrain::Serialize(Framework::Data::Scope& parentScope) const
{
	Data::Scope& myScope = parentScope.AddChild("Terrain");

	myScope.AddVariable("numOfChunksX") << mData->mNumOfChunksX;
	myScope.AddVariable("numOfChunksZ") << mData->mNumOfChunksZ;
	myScope.AddVariable("heightMap") << mData->GetHeightMap();
	myScope.AddVariable("seedNonRepeatTexture") << mSeedForNonRepeatTexture;
}

void Framework::Terrain::Deserialize(const Framework::Data::Scope& parentScope)
{
	std::optional<const Data::Scope*> myOptionalScope = parentScope.TryGetScope("Terrain");
	if (!myOptionalScope.has_value())
	{
		LOGWARNING("Could not load in terrain from file, data missing");
		return;
	}
	const Data::Scope& myScope = *myOptionalScope.value();

	uint numOfChunksX;
	uint numOfChunksZ;
	myScope.GetVariable("numOfChunksX") >> numOfChunksX;
	myScope.GetVariable("numOfChunksZ") >> numOfChunksZ;

	mData = std::make_unique<TerrainData>(numOfChunksX, numOfChunksZ);

	std::vector<float> heightMap{};
	myScope.GetVariable("heightMap") >> heightMap;
	mData->SetHeightMap(std::move(heightMap));

	myScope.GetVariable("seedNonRepeatTexture") >> mSeedForNonRepeatTexture;
	GenerateNoiseForNonRepeatTexture(mSeedForNonRepeatTexture);
}

void Framework::Terrain::OnSettingsChange(const Data::Scope& previousSettings, const Data::Scope& currentSettings)
{
	const Data::Variable& previousValue = previousSettings.GetVariable("maxTextureSize");
	const Data::Variable& currentValue = currentSettings.GetVariable("maxTextureSize");

	if (previousValue != currentValue)
	{
		GenerateNoiseForNonRepeatTexture(mSeedForNonRepeatTexture);
	}
}

void Framework::Terrain::SendTerrainToPhysics()
{
	if (mCollisionObject != nullptr)
	{
		LOGWARNING("Terrain has already been send to physics.");
		mScene.mPhysics->RemoveCollisionObjectFromWorld(std::move(mCollisionObject));
	}

	//assert(IsPowerOfTwo(mData->mNumOfVerticesX - 1)
	//	&& IsPowerOfTwo(mData->mNumOfVerticesZ - 1)
	//	&& "Bullet requires height maps to be a power of two + 1");

	//assert(mData->mNumOfVerticesX == mData->mNumOfVerticesZ
	//	&& "Bullet requires square sized maps");

	mCollisionShape = std::make_unique<btHeightfieldTerrainShape>(mData->mNumOfVerticesX, mData->mNumOfVerticesZ, &mData->GetHeightMap()[0], 1.0f, 0.0f, mData->GetHeighestVertexHeight(), 1, PHY_FLOAT, false);

	constexpr float scale = Chunk::sSpaceBetweenVertices;
	//assert(scale == 1.0f
	//	&& "There's a bug where there's no collision when the scale is not 1.0f. Fix this bug or set the vertex resolution to 1.0f");
	mCollisionShape->setLocalScaling({ scale, 1.0f, scale });
	// The ratio will be the same for the X and Z angle, so only calculate it once.
	//mCollisionTransform->SetLocalScale({ scale, 1.0f, scale});

	btCollisionObject object;
	object.setCollisionShape(mCollisionShape.get());

	Transform transform{};
	transform.SetLocalPosition({ mData->mWorldSizeX * 0.5f, mData->GetHeighestVertexHeight() * 0.5f, mData->mWorldSizeZ * 0.5f });
	object.setWorldTransform(transform.ToBullet());
	object.setFriction(1.0f);

	mCollisionObject = std::make_unique<btCollisionObject>(std::move(object));
	mScene.mPhysics->AddCollisionObjectToWorld(mCollisionObject.get(), Framework::Physics::Group::terrainGroup, Framework::Physics::Mask::terrainMask);
}