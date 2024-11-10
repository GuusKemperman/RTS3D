#include "precomp.h"
#include "Level.h"

#include <time.h>
#include <chrono>

#include "SavedData.h"
#include "Hills.h"
#include "Forest.h"
#include "Terrain.h"
#include "Scope.h"
#include "Army.h"
#include "Player.h"
#include "Opponent.h"
#include "game.h"
#include "MainMenu.h"
#include "InputManager.h"
#include "Sprite.h"
#include "AssetManager.h"
#include "ImguiHelpers.h"
#include "TimeManager.h"

RTS::Level::Level(Framework::Game& game, const std::string& levelFile, const std::string& levelName) :
	Scene(game, levelFile, levelName)
{
	mLevelGeneration = mSceneData->TryGetScope("LevelGeneration");
	mEndscreen = Framework::AssetManager::Inst().GetAsset<Framework::Sprite>("data/sprites/endscreen.txt");
}

RTS::Level::~Level() = default;

void RTS::Level::Tick()
{
	if (mPlayerArmy->NumOfUnitsSpawned() != 0
		&& mOpponentArmy->NumOfUnitsSpawned() != 0)
	{
		bool playerWon = mOpponentArmy->Size() == 0;
		bool opponentWon = mPlayerArmy->Size() == 0;

		if ((playerWon || opponentWon)
			&& mVictoryState == VictoryState::none)
		{
			if (playerWon)
			{
				mVictoryState = VictoryState::playerWon;
			}
			else if (opponentWon)
			{
				mVictoryState = VictoryState::opponentWon;
			}

			mEndscreen->SetFrame(static_cast<uint>(mVictoryState));

			if (!mIsPaused)
			{
				TogglePause();
			}

			Framework::TimeManager::SetTimeScale(.2f);
		}
	}

	if (Framework::InputManager::GetInput(Framework::InputId::ESC).down
		&& mVictoryState != VictoryState::none)
	{
		TogglePause();
	}

	if (!mIsPaused)
	{
		Scene::Tick();
	}
}

uchar RTS::Level::Deserialize(const uchar progress)
{
	if (mLevelGeneration.has_value())
	{
		const Framework::Data::Scope* levelData = mLevelGeneration.value();

		if (progress == 0)
		{
			const Framework::Data::Scope& terrainScope = levelData->GetScope("Terrain");

			glm::u32vec2 numOfChunks;
			uint seed;
			float maxHeight;
			float roughness;
			float persistence;
			uint octaves;

			terrainScope.GetVariable("numOfChunks") >> numOfChunks;
			terrainScope.GetVariable("seed") >> seed;
			terrainScope.GetVariable("maxHeight") >> maxHeight;
			terrainScope.GetVariable("roughness") >> roughness;
			terrainScope.GetVariable("persistence") >> persistence;
			terrainScope.GetVariable("octaves") >> octaves;

			Framework::Random::Seed(seed);

			std::unique_ptr<Hills> hills = std::make_unique<Hills>(
				numOfChunks.x,
				numOfChunks.y,
				maxHeight,
				roughness,
				persistence,
				octaves
				);

			mTerrain->SetTerrainData(std::move(hills));
			
			const Framework::Data::Scope& forestScope = levelData->GetScope("Forest");
			bool forestEnabled;
			forestScope.GetVariable("enabled") >> forestEnabled;
			if (forestEnabled)
			{
				float forestSize; 
				float worldCoverage;
				float maxSteepness;

				forestScope.GetVariable("forestSize") >> forestSize;
				forestScope.GetVariable("worldCoverage") >> worldCoverage;
				forestScope.GetVariable("maxSteepness") >> maxSteepness;

				mForest = std::make_unique<Forest>(*this, forestSize, worldCoverage, maxSteepness);

				mForest->StartCalculatingLocations();
			}

			return 1;
		}
		else if (progress <= 1)
		{
			uint seed;
			levelData->GetVariable("Terrain.seed") >> seed;
			mTerrain->GenerateNoiseForNonRepeatTexture(seed);
			return 2;
		}
		else if (progress <= 30)
		{
			constexpr uchar progressStart = 2;
			constexpr uchar progressEnd = 30;
			const float dataProgress = mTerrain->GetData()->GenerateHeightMap(2500);

			if (dataProgress == 1.0f)
			{
				return progressEnd + 1;
			}
			return static_cast<uchar>(Framework::Math::lerp(static_cast<float>(progressStart), static_cast<float>(progressEnd), dataProgress));
		}
		else if (progress <= 69)
		{
			constexpr uchar progressStart = 32;
			constexpr uchar progressEnd = 69;

			const float terrainProgress = mTerrain->GenerateTerrain(1);

			if (terrainProgress == 1.0f)
			{
				return progressEnd + 1;
			}
			return static_cast<uchar>(Framework::Math::lerp(static_cast<float>(progressStart), static_cast<float>(progressEnd), terrainProgress));
		}
		else if (progress <= 89)
		{
			constexpr uchar progressStart = 70;
			constexpr uchar progressEnd = 89;

			if (mForest != nullptr)
			{
				const float forestProgress = mForest->SpawnTrees(100);
				if (forestProgress == 1.0f)
				{
					mForest.reset(); // No longer need it, free this memory.
					return progressEnd + 1;
				}
				return static_cast<uchar>(Framework::Math::lerp(static_cast<float>(progressStart), static_cast<float>(progressEnd), forestProgress));
			}
			return progressEnd + 1;
		}
		else if (progress <= 90)
		{
			const Framework::Data::Scope& playerScope = levelData->GetScope("Player");

			mPlayerArmy = &mEntityManager->AddEntity<Army>(ArmyId::player);
			mPlayerArmy->SpawnUnits(playerScope.GetScope("Army"));
			mEntityManager->AddEntity<Player>(mPlayerArmy->GetId());

			return 96;
		}
		else
		{
			const Framework::Data::Scope& opponentScope = levelData->GetScope("Opponent");

			mOpponentArmy = &mEntityManager->AddEntity<Army>(ArmyId::opponent);
			mOpponentArmy->SpawnUnits(opponentScope.GetScope("Army"));
			mEntityManager->AddEntity<Opponent>(mOpponentArmy->GetId());

			return 100;
		}
	}
	else
	{
		uchar newProgress = Scene::Deserialize(progress);

		if (newProgress == 100)
		{
			std::vector<Army*> armies = mEntityManager->GetEntities<Army>();
			assert(armies.size() == 2);

			if (armies[0]->GetArmyId() == ArmyId::player)
			{
				mPlayerArmy = armies[0];
				mOpponentArmy = armies[1];
			}
			else
			{
				mPlayerArmy = armies[1];
				mOpponentArmy = armies[0];
			}
		}

		return newProgress;
	}
}

void RTS::Level::DrawImGui()
{
	constexpr ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoScrollbar;

	if (mIsPaused)
	{
		if (mVictoryState == VictoryState::none)
		{
			constexpr ImVec2 windowSize = { sScreenHeight * 0.5f, sScreenWidth * 0.5f };
			constexpr ImVec2 windowPosition = { sHalfScreenWidth - windowSize.x / 2, sHalfScreenHeight - windowSize.y / 2 };
			ImGui::SetNextWindowSize(windowSize);
			ImGui::SetNextWindowPos(windowPosition);

			if (ImGui::Begin("Pause window", nullptr, windowFlags))
			{
				ImGui::InputText("Save name", &mWhatToNameTheSave);

				if (ImGui::Button("Save"))
				{
					Serialize(mWhatToNameTheSave);
				}

				if (ImGui::Button("Return to main menu"))
				{
					mGame.RequestLoadTo(std::make_unique<MainMenu>(mGame));
				}
			}
			ImGui::End();
		}
		else
		{
			constexpr ImVec2 windowSize = { sScreenWidth, sScreenHeight };
			constexpr ImVec2 windowPosition = { 0, 0 };
			ImGui::SetNextWindowSize(windowSize);
			ImGui::SetNextWindowPos(windowPosition);
			ImGui::SetNextWindowBgAlpha(0.0f);

			// We're doing it like this because the push/pop style var was giving me issues, and it's such a small thing that i cant be bothered with figuring out why when i can just do this quick fix
			ImGuiStyle& style = ImGui::GetStyle();
			const float defaultBorderSize = style.WindowBorderSize;
			style.WindowBorderSize = 0.0f;

			if (ImGui::Begin("Victory screen", nullptr, windowFlags))
			{
				const ImVec2 imageSize = Framework::Math::ToIMGui(mEndscreen->GetFrameSizePixels());
				const ImVec2 imagePos = Framework::ImguiHelpers::Centre(windowSize, imageSize);
				ImGui::SetCursorPos(imagePos);
				mEndscreen->DrawImGui();

				const ImVec2 buttonSize = { 200, 50 };
				ImGui::SetCursorPos({ imagePos.x + Framework::ImguiHelpers::Centre(imageSize.x, buttonSize.x), imagePos.y + imageSize.y + 10.0f });
				if (ImGui::Button("Return to main menu", buttonSize))
				{
					mGame.RequestLoadTo(std::make_unique<MainMenu>(mGame));
				}
			}
			ImGui::End();

			style.WindowBorderSize = defaultBorderSize;
		}
	}

	constexpr ImVec2 windowSize = { 400, 100.0f };
	ImGui::SetNextWindowSize(windowSize);
	ImGui::SetNextWindowPos({ (sScreenWidth - windowSize.x) * 0.5f, 0 });
	ImGui::SetNextWindowBgAlpha(0.0f);

	// We're doing it like this because the push/pop style var was giving me issues, and it's such a small thing that i cant be bothered with figuring out why when i can just do this quick fix
	ImGuiStyle& style = ImGui::GetStyle();
	float defaultBorderSize = style.WindowBorderSize;
	style.WindowBorderSize = 0.0f;

	if (ImGui::Begin("InGameUI", nullptr, windowFlags))
	{
		Framework::ImguiHelpers::SetWindowFontSize(40.0f);

		constexpr ImVec2 pauseButtonSize = { windowSize.y - 20.0f, windowSize.y - 20.0f };
		constexpr ImVec2 pauseButtonPos = { (windowSize.x - pauseButtonSize.x) * 0.5f, (windowSize.y - pauseButtonSize.y) * 0.5f };

		{
			std::string armySize = std::to_string(mPlayerArmy->Size());
			ImVec2 txtSize = ImGui::CalcTextSize(armySize.c_str());

			constexpr float centreOn = (pauseButtonPos.x) * 0.5f;
			ImGui::SetCursorPos({centreOn - txtSize.x * .5f, (windowSize.y - txtSize.y) * 0.5f});
			ImGui::TextColored({ 0.0f, 0.0f, 1.0f, 1.0f }, armySize.c_str());
		}

		{
			std::string armySize = std::to_string(mOpponentArmy->Size());
			ImVec2 txtSize = ImGui::CalcTextSize(armySize.c_str());

			constexpr float centreOn = (windowSize.x) - pauseButtonPos.x * .5f;
			ImGui::SetCursorPos({ centreOn - txtSize.x * .5f, (windowSize.y - txtSize.y) * 0.5f });
			ImGui::TextColored({ 1.0f, 0.0f, 0.0f, 1.0f }, armySize.c_str());
		}

		{
			ImGui::SetCursorPos(pauseButtonPos);
			ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.2f, .5f, 1.0f });
			if (ImGui::Button("||", pauseButtonSize))
			{
				TogglePause();
			}
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();

	style.WindowBorderSize = defaultBorderSize;
}

void RTS::Level::Unload()
{
	Framework::TimeManager::SetTimeScale(1.0f);
}

std::string RTS::Level::GenerateSaveName() const
{
	std::string saveName{};

	// https://stackoverflow.com/questions/34857119/how-to-convert-stdchronotime-point-to-string
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm{};
	
#ifdef PLATFORM_LINUX
	localtime_r(&now_c, &now_tm);
#elif PLATFORM_WINDOWS
	localtime_s(&now_tm, &now_c);
#else
	static_assert(false, "No local time function here for this platform!");
#endif // PLATFORM_LINUX


	char buff[70]{};
	if (strftime(buff, sizeof buff, "[%d-%m-%y-%H-%M-%S] ", &now_tm))
	{
		saveName.append(buff);
	}
	
	std::string levelName = mSceneData->GetScope().GetName();

	size_t firstClosingBracket = levelName.find_first_of(']');

	if (firstClosingBracket != std::string::npos)
	{
		levelName = levelName.substr(firstClosingBracket + 1);
	}

	saveName += levelName;

	return saveName;
}

void RTS::Level::TogglePause()
{
	mIsPaused = !mIsPaused;

	if (mIsPaused)
	{
		mWhatToNameTheSave = GenerateSaveName();
	}
}