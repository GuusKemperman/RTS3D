#include "precomp.h"
#include "Scene.h"

#include "game.h"
#include "EntityManager.h"
#include "Camera.h"
#include "Terrain.h"
#include "Physics.h"
#include "SavedData.h"
#include "TimeManager.h"

Framework::Scene::Scene(Game& game, const std::string& levelFile, const std::string& levelName) :
	mGame(game)
{
	mPhysics = std::make_unique<Physics>(*this);
	mCamera = std::make_unique<Camera>(*this);
	mTerrain = std::make_unique<Terrain>(*this);
	mEntityManager = std::make_unique<EntityManager>(*this);

	if (!levelFile.empty())
	{
		mSceneData = std::make_unique<Data::SavedData>(levelFile, levelName);
	}
}

Framework::Scene::~Scene() = default;

uchar Framework::Scene::Deserialize(const uchar progress)
{
	if (mSceneData == nullptr)
	{
		return 100;
	}

	const Data::Scope& myScope = mSceneData->GetScope();

	if (progress <= 4)
	{
		mCamera->Deserialize(myScope);
		return 5;
	}
	else if (progress <= 29)
	{
		mTerrain->Deserialize(myScope);
		return 30;
	}
	else if (progress <= 80)
	{
		constexpr uchar progressStart = 30;
		constexpr uchar progressEnd = 80;

		const float entityProgress = mEntityManager->Deserialize(myScope, 5);

		if (entityProgress == 1.0f)
		{
			return progressEnd + 1;
		}
		return static_cast<uchar>(Math::lerp(static_cast<float>(progressStart), static_cast<float>(progressEnd), entityProgress));
	}
	else if (progress < 100)
	{
		constexpr uchar progressStart = 81;
		constexpr uchar progressEnd = 99;

		const float terrainProgress = mTerrain->GenerateTerrain(1);

		if (terrainProgress == 1.0f)
		{
			return progressEnd + 1;
		}
		return static_cast<uchar>(Math::lerp(static_cast<float>(progressStart), static_cast<float>(progressEnd), terrainProgress));
	}
	return 100; // Just to silence warning
}

void Framework::Scene::Tick()
{
	mEntityManager->DeconstructDestroyedEntities();
	mEntityManager->Tick();
	mPhysics->Tick();
}

void Framework::Scene::Draw()
{
	mCamera->DrawScene();
	DrawImGui();
}

void Framework::Scene::Unload()
{
	mEntityManager->Clear();
	mEntityManager.reset();
	mCamera.reset();
	mTerrain.reset();
	mPhysics.reset();

	TimeManager::SetTimeScale(1.0f);
}

void Framework::Scene::Serialize(const std::string& saveName) const
{
	std::string filepath = "saves/" + saveName + ".dat";

	Framework::Data::SavedData::MakeEmpty(filepath);
	Framework::Data::SavedData saveData = { filepath };
	Framework::Data::Scope& globalScope = saveData.GetScope();

	Data::Scope& myScope = globalScope.AddChild(saveName);

	Serialize(myScope);

	saveData.Save();
}

void Framework::Scene::Serialize(Data::Scope& myScope) const
{
	mCamera->Serialize(myScope);
	mTerrain->Serialize(myScope);
	mEntityManager->Serialize(myScope);
}