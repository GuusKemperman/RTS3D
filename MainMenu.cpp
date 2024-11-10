#include "precomp.h"
#include "MainMenu.h"

#include <filesystem>

#include "game.h"
#include "Army.h"
#include "Terrain.h"
#include "TerrainData.h"
#include "TimeManager.h"
#include "Camera.h"
#include "Opponent.h"
#include "SavedData.h"
#include "Scope.h"
#include "Sandbox.h"
#include "Level.h"
#include "EntityManager.h"
#include "Player.h"
#include "SavedData.h"
#include "AssetManager.h"
#include "ImGuiFontWrapper.h"
#include "Settings.h"
#include "Texture.h"
#include "ImguiHelpers.h"

RTS::MainMenu::MainMenu(Framework::Game& game) :
	Level(game, "mainmenu.txt", "mainmenu")
{
}

RTS::MainMenu::~MainMenu() = default;

uchar RTS::MainMenu::Deserialize(const uchar progress)
{
	const uchar newProgress = Level::Deserialize(progress);

	if (newProgress == 100)
	{
		mMainTitleFont = Framework::AssetManager::Inst().GetAsset<Framework::ImGuiFontWrapper>("fonts/ALGER.TTF,200.0f");

		{
			const std::filesystem::path levelsPath = sDataRoot + "levels";
			const size_t levelsPathSize = levelsPath.string().size();

			for (const std::filesystem::directory_entry& dirEntry : std::filesystem::directory_iterator{ levelsPath })
			{
				const std::string entryString = dirEntry.path().string();
				const size_t nameSize = entryString.size() - levelsPathSize - 5;
				std::string name = entryString.substr(levelsPathSize + 1, nameSize);
				mLevelNames.push_back(std::move(name));
			}
		}

		{
			const std::filesystem::path savesPath = sDataRoot +"saves";
			const size_t savesPathSize = savesPath.string().size();

			for (const std::filesystem::directory_entry& dirEntry : std::filesystem::directory_iterator{ savesPath })
			{
				const std::string entryString = dirEntry.path().string();
				const size_t nameSize = entryString.size() - savesPathSize - 5;
				std::string name = entryString.substr(savesPathSize + 1, nameSize);
				mSavesNames.push_back(std::move(name));
			}
		}

		// We don't actually need the player to control the camera, so let's get rid of them.
		std::vector<Player*> players = mEntityManager->GetEntities<Player>();

		for (Player* player : players)
		{
			player->Destroy();
		}
	}

	return newProgress;
}

void RTS::MainMenu::Tick()
{
	Framework::Transform& cameraTransform = mCamera->GetTransform();
	cameraTransform.TranslateLocalPosition(mCamDir * sCamMoveSpeed * Framework::TimeManager::GetDeltaTime());

	float currentTime = Framework::TimeManager::GetTotalTimePassed();
	float timeSinceLastDirChanged = currentTime - mTimeCamDirChanged;
	if (timeSinceLastDirChanged >= sTimeBetweenCamDirChange)
	{
		cameraTransform.SetLocalPosition(RandomCameraPosition());
		mCamDir = Framework::Math::AngleToVec2(Framework::Random::Range(360.0f));
		mTimeCamDirChanged = currentTime;
		//ReplenishArmies();
	}

	Scene::Tick();
}

void RTS::MainMenu::DrawImGui()
{
	constexpr ImGuiWindowFlags genericWindowFlags =
		ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoTitleBar;

	constexpr ImVec2 genericWindowSize = { 800, 800 };
	constexpr ImVec2 genericWindowPosition = { sHalfScreenWidth - genericWindowSize.x / 2, sHalfScreenHeight - genericWindowSize.y / 2 };

	constexpr ImVec2 genericButtonSize = { 120.0f, 40.0f };
	constexpr float genericSpacing = 10.0f;

	constexpr ImVec2 wideButtonSize = { genericWindowSize.x - genericSpacing * 2, 30.0f };

	constexpr float genericContentFontSize = Framework::ImguiHelpers::sDefaultFontSize * 1.5f;
	constexpr float genericNavigationButtonsFontSize = genericContentFontSize * 1.5f;
	
	switch (mActiveMenu)
	{
	case RTS::MainMenu::Menu::main:
	{
		constexpr ImVec2 windowSize = { sScreenWidth, sScreenHeight };
		constexpr ImVec2 windowPosition = { sHalfScreenWidth - windowSize.x / 2, sHalfScreenHeight - windowSize.y / 2 };

		ImGui::SetNextWindowSize(windowSize);
		ImGui::SetNextWindowPos(windowPosition);
		ImGui::SetNextWindowBgAlpha(0.0f);

		if (ImGui::Begin("Mainmenutest", nullptr, genericWindowFlags))
		{
			{
				ImGui::PushFont(mMainTitleFont->GetFont());
				mMainTitleFont->SetWindowFontSize(200.0f);

				const char* title = "Retro Warfare!";

				float textWidth = ImGui::CalcTextSize(title).x;
				ImGui::SetCursorPosX((windowSize.x - textWidth) * 0.5f);
				ImGui::Text(title);
				ImGui::PopFont();
			}

			{
				Framework::ImguiHelpers::SetWindowFontSize(20.0f);
				const char* credits = "Made by Guus Kemperman";
				float textWidth = ImGui::CalcTextSize(credits).x;
				ImGui::SetCursorPosX((windowSize.x - textWidth) * 0.5f);
				ImGui::Text(credits);
			}

			constexpr ImVec2 buttonSize = { windowSize.x * .25f, windowSize.y * 0.075f };
			constexpr ImVec2 buttonStart = { (windowSize.x - buttonSize.x) * 0.5f, windowSize.y * 0.3f };
			constexpr ImVec4 buttonColor = { 0.1f, 0.1f, 0.3f, .9f };
			constexpr float spacing = windowSize.y * 0.05f;

			ImGui::SetCursorPos(buttonStart);
			ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
			Framework::ImguiHelpers::SetWindowFontSize(Framework::ImguiHelpers::sDefaultFontSize * 2.0f);

			
			if (ImGui::Button("New game", buttonSize))
			{
				mActiveMenu = Menu::level;
			}
			
			ImGui::SetCursorPos({ buttonStart.x, buttonStart.y + buttonSize.y + spacing });
			if (ImGui::Button("Load", buttonSize))
			{
				mActiveMenu = Menu::saves;
			}

			ImGui::SetCursorPos({ buttonStart.x, buttonStart.y + 2.0f * (buttonSize.y + spacing) });
			if (ImGui::Button("Settings", buttonSize))
			{
				mNewSettings = std::make_unique<Framework::Data::Scope>(Framework::Settings::Inst().GetSettings());
				mActiveMenu = Menu::settings;
			}
			
			ImGui::SetCursorPos({ buttonStart.x, buttonStart.y + 3.0f * (buttonSize.y + spacing) });
			if (ImGui::Button("Quit to desktop", buttonSize))
			{
				mGame.Quit();
			}
			
			ImGui::PopStyleColor();
			Framework::ImguiHelpers::SetWindowFontSize();
		}
		ImGui::End();
		break;
	}
	case RTS::MainMenu::Menu::level:
	{
		ImGui::SetNextWindowSize(genericWindowSize);
		ImGui::SetNextWindowPos(genericWindowPosition);
		if (ImGui::Begin("Level selector", nullptr, genericWindowFlags))
		{
			Framework::ImguiHelpers::SetWindowFontSize(genericContentFontSize);

			for (int i = 0; i < static_cast<int>(mLevelNames.size()); i++)
			{
				if (ImGui::Selectable(mLevelNames[i].c_str(), mSelectedLevel == i, 0, wideButtonSize))
				{
					mSelectedLevel = i;
				}
			}
			
			Framework::ImguiHelpers::SetWindowFontSize(genericNavigationButtonsFontSize);

			if (mSelectedLevel != -1)
			{
				const std::string levelFile = "levels/" + mLevelNames[mSelectedLevel] + ".txt";
				const std::string& levelName = mLevelNames[mSelectedLevel];

				ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 5.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });
				if (ImGui::Button("Start", genericButtonSize))
				{
					mGame.RequestLoadTo(std::make_unique<Level>(mGame, levelFile, levelName));
				}

				ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 4.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });
				if (ImGui::Button("Delete", genericButtonSize)
					&& mSelectedLevel != -1)
				{
					Framework::Data::SavedData::Delete(levelFile);
					mLevelNames.erase(mLevelNames.begin() + mSelectedLevel);
					mSelectedLevel = -1;
				}

				ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 3.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });
				if (ImGui::Button("Edit", genericButtonSize))
				{
					mGame.RequestLoadTo(std::make_unique<Sandbox>(mGame, levelFile, levelName));
				}
			}

			ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 2.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });
			if (ImGui::Button("New", genericButtonSize))
			{
				mGame.RequestLoadTo(std::make_unique<Sandbox>(mGame, "sandbox.txt", "sandbox", true));
			}

			ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 1.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });
			if (ImGui::Button("Exit", genericButtonSize))
			{
				mActiveMenu = Menu::main;
				mSelectedLevel = -1;
			}
		}
		ImGui::End();
	}
		break;
	case RTS::MainMenu::Menu::saves:
	{
		ImGui::SetNextWindowSize(genericWindowSize);
		ImGui::SetNextWindowPos(genericWindowPosition);

		if (ImGui::Begin("Save selector", nullptr, genericWindowFlags))
		{
			Framework::ImguiHelpers::SetWindowFontSize(genericContentFontSize);

			for (int i = 0; i < static_cast<int>(mSavesNames.size()); i++)
			{
				if (ImGui::Selectable(mSavesNames[i].c_str(), mSelectedSave == i, 0, wideButtonSize))
				{
					mSelectedSave = i;
				}
			}

			Framework::ImguiHelpers::SetWindowFontSize(genericNavigationButtonsFontSize);
			ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 3.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });

			if (ImGui::Button("Start", genericButtonSize)
				&& mSelectedSave != -1)
			{
				const std::string file = "saves/" + mSavesNames[mSelectedSave] + ".dat";
				mGame.RequestLoadTo(std::make_unique<Level>(mGame, file, mSavesNames[mSelectedSave]));
			}

			ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 2.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });

			if (ImGui::Button("Delete", genericButtonSize)
				&& mSelectedSave != -1)
			{
				Framework::Data::SavedData::Delete(std::string{ "saves/" } + mSavesNames[mSelectedSave] + ".dat");
				mSavesNames.erase(mSavesNames.begin() + mSelectedSave);
				mSelectedSave = -1;
			}

			ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 1.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });

			if (ImGui::Button("Exit", genericButtonSize))
			{
				mActiveMenu = Menu::main;
				mSelectedSave = -1;
			}
		}
		ImGui::End();
	}
		break;
	case RTS::MainMenu::Menu::settings:
	{
		ImGui::SetNextWindowSize(genericWindowSize);
		ImGui::SetNextWindowPos(genericWindowPosition);

		if (ImGui::Begin("Settings", nullptr, genericWindowFlags))
		{
			Framework::ImguiHelpers::SetWindowFontSize(genericContentFontSize);

			Framework::Data::Scope& settingsScope = *mNewSettings;

			{
				bool tmpBool;
				Framework::Data::Variable& var = settingsScope.GetVariable("CPU-based Culling");
				var >> tmpBool;

				if (ImGui::Checkbox("CPU-based Culling", &tmpBool))
				{
					var << tmpBool;
				}

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Recommended for devices with slow GPUs.");
				}
			}

			{
				int currentValue;
				Framework::Data::Variable& var = settingsScope.GetVariable("maxTextureSize");
				var >> currentValue;

				int largestTextureSize = Framework::Texture::GetLargestTextureLoadedInSize();
				int newValue = std::min(currentValue, largestTextureSize);
				if (ImGui::SliderInt("Max texture size", &newValue, 1, std::min(largestTextureSize, Framework::Texture::GetMaxTextureSizeForDevice())))
				{
					var << static_cast<int>(newValue);
				}
			}

			{
				float currentValue;
				Framework::Data::Variable& var = settingsScope.GetVariable("fontQuality");
				var >> currentValue;

				float maxValue;
				settingsScope.GetVariable("maxFontQuality") >> maxValue;

				if (ImGui::SliderFloat("Font quality", &currentValue, 0.01f, maxValue))
				{
					var << currentValue;
				}
			}

			{
				float currentValue;
				Framework::Data::Variable& var = settingsScope.GetVariable("specularIntensity");
				var >> currentValue;

				if (ImGui::SliderFloat("Specular intensity", &currentValue, 0.0f, 1.0f))
				{
					var << currentValue;
				}
			}

			Framework::ImguiHelpers::SetWindowFontSize(genericNavigationButtonsFontSize);

			ImGui::SetCursorPos({ genericWindowSize.x - (genericButtonSize.x + genericSpacing) * 1.0f, genericWindowSize.y - genericSpacing - genericButtonSize.y });

			if (ImGui::Button("Exit", genericButtonSize))
			{
				Framework::Settings::Inst().SetSettings(*mNewSettings);
				mActiveMenu = Menu::main;
			}
		}
		ImGui::End();
	}
		break;
	default:
		break;
	}

	static std::optional<bool> showControls{};

	if (!showControls.has_value())
	{
		const Framework::Data::Scope& settingsScope = Framework::Settings::Inst().GetSettings();

		bool tmpBool;
		settingsScope.GetVariable("showControlsOnStart") >> tmpBool;

		showControls = tmpBool;
	}

	if (!showControls.value())
	{
		return;
	}

	constexpr ImVec2 welcomeSize = { 500, 500 };
	ImGui::SetNextWindowSize(welcomeSize);
	ImGui::SetNextWindowPos(Framework::ImguiHelpers::Centre({ sScreenWidth, sScreenHeight }, welcomeSize));
	ImGui::SetNextWindowFocus();

	if (ImGui::Begin("Welcome!", &showControls.value()))
	{
		constexpr const char* info = "Start, delete or edit existing levels using \"Start\" button."
			"You can also create new levels from here. While playing a level, you can pause to save the game."
			"You can continue playing where you left of from using the \"Load\" button.\n\n"
			"Pause: ESC\n"
			"Movement: WASD\n"
			"Sprint: Shift\n"
			"CameraHeight: CTRL/SPACE\n"
			"Orbit camera: Q/E\n"
			"Select: Click/drag left mouse\n"
			"Deselect: LeftMouse\n"
			"Command selection: RightMouse\n";
		ImGui::TextWrapped(info);

		static bool doNotShow{};
		if (ImGui::Checkbox("Do not show me again", &doNotShow))
		{
			Framework::Data::Scope settingsScope = Framework::Settings::Inst().GetSettings();
			settingsScope.GetVariable("showControlsOnStart") << !doNotShow;
			Framework::Settings::Inst().SetSettings(settingsScope);
		}
	}
	ImGui::End();
}

void RTS::MainMenu::ReplenishArmies() const
{
	//Army* armies[2]{ mPlayerArmy, mOpponentArmy };

	//const int numOfUnitsXY = static_cast<int>(sqrt(static_cast<double>(sDesiredArmySize)));
	//const float spacing = 3.0f;

	//const Framework::TerrainData* const terrainData = mTerrain->GetData();
	//const glm::vec3 worldSize = { terrainData->mWorldSizeX, 0.0f, terrainData->mWorldSizeZ };

	//for (int armyNum = 0; armyNum < 2; armyNum++)
	//{
	//	Army* army = armies[armyNum];
	//	uint armySize = static_cast<uint>(army->GetIds().size());

	//	for (int x = 0; x < numOfUnitsXY && armySize < sDesiredArmySize; x++)
	//	{
	//		for (int y = 0; y < numOfUnitsXY && armySize < sDesiredArmySize; y++)
	//		{
	//			glm::vec3 spawnPos(static_cast<float>(x) * spacing + spacing, 0.0f, static_cast<float>(y) * spacing + spacing);

	//			if (armyNum == 1)
	//			{
	//				spawnPos = worldSize - spawnPos;
	//			}


	//			army->AddToArmy<Tank>().GetTransform().SetLocalPosition(spawnPos);

	//			armySize++;
	//		}
	//	}
	//}
}

glm::vec2 RTS::MainMenu::RandomCameraPosition() const
{
	const Framework::TerrainData* const terrainData = mTerrain->GetData();
	return { Framework::Random::Range(sMinCamDistFromEdges, terrainData->mWorldSizeX - sMinCamDistFromEdges), 
		Framework::Random::Range(sMinCamDistFromEdges, terrainData->mWorldSizeZ - sMinCamDistFromEdges )};
}
