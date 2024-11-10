#include "precomp.h"
#include "Sandbox.h"

#include "game.h"
#include "EntityManager.h"
#include "Army.h"
#include "Terrain.h"
#include "Hills.h"
#include "Forest.h"
#include "Player.h"
#include "SavedData.h"
#include "Scope.h"
#include "Camera.h"
#include "MainMenu.h"
#include "StringFunctions.h"
#include "ImguiHelpers.h"
#include "ProceduralUnitFactory.h"
#include "AssetManager.h"
#include "Physics.h"

RTS::Sandbox::Sandbox(Framework::Game& game, const std::string& levelFile, const std::string& levelName, const bool generateNewLevelName, const bool hasBeenSaved) :
	Level(game, levelFile, levelName),
	mHasBeenSaved(hasBeenSaved)
{
	const Framework::Data::Scope* scopeUsedForLevelGeneration = GetScopeUsedForLevelGeneration().value();
	mParamatersScope = std::make_unique<Framework::Data::Scope>(*scopeUsedForLevelGeneration);
	mParamatersScope->SetName(generateNewLevelName ? GenerateLevelName() : scopeUsedForLevelGeneration->GetParent()->GetName());
	mProceduralUnitFactory = Framework::AssetManager::Inst().GetAsset<ProceduralUnitFactory>(sDataRootWithoutAssetRoot + "procedural");
}

RTS::Sandbox::~Sandbox() = default;

uchar RTS::Sandbox::Deserialize(const uchar progress)
{	
	const uchar newProgress = Level::Deserialize(progress);

	if (newProgress == 100)
	{
		mPlayer = mEntityManager->GetEntities<Player>().front();
		// Tick only once so the frustum culling works properly
		mPhysics->Tick();
	}
	return newProgress;
}

void RTS::Sandbox::Tick()
{
	mPlayer->Tick();
}

constexpr ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen;

void DivideOverMultipleFrequencies(const float amountToAdd, std::vector<Framework::Data::Variable>& data, std::list<size_t> exempt)
{
	float totalAmount = amountToAdd;

	for (size_t iteration = 0; iteration < data.size() * 5 && totalAmount != 0.0f && data.size() != exempt.size(); iteration++)
	{
		float additionPerEntry = totalAmount / static_cast<float>(data.size() - exempt.size());

		if (additionPerEntry == 0.0f)
		{
			additionPerEntry = totalAmount;
		}

		// Ensure that the total value is no greater than 1.0
		for (size_t j = 0; j < data.size() && totalAmount != 0.0f; j++)
		{
			if (std::find(exempt.begin(), exempt.end(), j) != exempt.end())
			{
				continue;
			}

			Framework::Data::Variable& otherNameAndFrequency = data[j];
			float initialFrequency;
			otherNameAndFrequency >> initialFrequency;

			float updatedFrequency = initialFrequency + additionPerEntry;

			if (updatedFrequency < 0.0f
				|| updatedFrequency > 1.0f)
			{
				// We can't add anything more to this, so we need to divide it over the others
				updatedFrequency = static_cast<float>(updatedFrequency > 1.0f);
				exempt.push_back(j);
			}


			otherNameAndFrequency << updatedFrequency;
			totalAmount += initialFrequency - updatedFrequency;
		}
	}
}

bool DrawPartFrequencies(Framework::Data::Scope& frequencies, const size_t idStart)
{
	bool changeMade = false;
	std::vector<Framework::Data::Variable>& nameAndFrequencies = frequencies.GetVariables();

	for (size_t i = 0; i < nameAndFrequencies.size(); i++)
	{
		ImGui::PushID(static_cast<int>(i + idStart));
		Framework::Data::Variable& nameAndFrequency = nameAndFrequencies[i];

		float frequency;
		nameAndFrequency >> frequency;

		float newFrequency = frequency;
		if (ImGui::SliderFloat(nameAndFrequency.GetName().c_str(), &newFrequency, 0.0f, 1.0f))
		{
			nameAndFrequency << newFrequency;

			const float difference = newFrequency - frequency;
			DivideOverMultipleFrequencies(-difference, nameAndFrequencies, { i });
			changeMade = true;
		}
		ImGui::PopID();
	}
	return changeMade;
}

bool RTS::Sandbox::DrawArmyConfiguration(Framework::Data::Scope& owner)
{
	bool changesMade = false;

	if (!ImGui::CollapsingHeader(owner.GetName().c_str(), treeNodeFlags))
	{
		return false;
	}

	Framework::Data::Scope& army = owner.GetScope("Army");
	std::vector<Framework::Data::Scope>& groups = army.GetChildren();
		
	for (size_t i = 0; i < groups.size(); i++)
	{
		ImGui::PushID(static_cast<int>(i + 10000));
		Framework::Data::Scope& group = groups[i];

		if (ImGui::TreeNode(group.GetName().c_str()))
		{
			{
				Framework::Data::Variable& amount = group.GetVariable("amount");
				int intTmp;
				amount >> intTmp;
				if (ImGui::InputInt("Amount", &intTmp))
				{
					amount << static_cast<int>(std::max(intTmp, 0));
					changesMade = true;
				}
			}

			{
				Framework::Data::Variable& position = group.GetVariable("centre");
				glm::vec2 vec2Tmp;
				position >> vec2Tmp;

				if (ImGui::InputFloat2("Position", &vec2Tmp[0]))
				{
					position << vec2Tmp;
					changesMade = true;
				}
			}

			if (ImGui::TreeNode("Body Frequencies"))
			{
				if (DrawPartFrequencies(group.GetScope("bodies"), 20000))
				{
					changesMade = true;
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Turret Frequencies"))
			{
				if (DrawPartFrequencies(group.GetScope("turrets"), 30000))
				{
					changesMade = true;
				}
				ImGui::TreePop();
			}

			if (ImGui::Button("Remove"))
			{
				groups.erase(groups.begin() + i);
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}
	
	if (ImGui::Button("+"))
	{
		uint seed = rand();
		std::string randomName = Framework::Random::GenerateRandomPronounceableString(Framework::Random::Range(3u, 6u), seed) + "'s battalion";
		Framework::Data::Scope& newGroup = army.AddChild(std::move(randomName));
		newGroup.AddVariable("amount") << 0;
		newGroup.AddVariable("centre") << glm::vec2{};

		Framework::Data::Scope& bodyFrequencies = newGroup.AddChild("bodies");
		const std::vector<UnitBodyData>& bodies = mProceduralUnitFactory->GetBodies();
		float frequency = 1.0f / static_cast<float>(bodies.size());
		for (const UnitBodyData& body : bodies)
		{
			bodyFrequencies.AddVariable(body.mName) << frequency;
		}

		Framework::Data::Scope& turretFrequencies = newGroup.AddChild("turrets");
		const std::vector<TurretData>& turrets = mProceduralUnitFactory->GetTurrets();
		frequency = 1.0f / static_cast<float>(turrets.size());
		for (const TurretData& turret : turrets)
		{
			turretFrequencies.AddVariable(turret.mName) << frequency;
		}
	}
	return changesMade;
}

void RTS::Sandbox::DrawImGui()
{
	constexpr ImVec2 windowSize = { sScreenWidth, sScreenHeight };
	constexpr ImVec2 windowPosition = { 0, 0 };
	ImGui::SetNextWindowSize(windowSize);
	ImGui::SetNextWindowPos(windowPosition);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoSavedSettings;

	if (!mHasBeenSaved)
	{
		windowFlags |= ImGuiWindowFlags_UnsavedDocument;
	}

	if (mCollapseWindow)
	{
		ImGui::SetNextWindowCollapsed(true);
		mCollapseWindow = false;
	}
	if (ImGui::Begin("Configure Sandbox", nullptr, windowFlags))
	{
		constexpr ImVec2 contentSize = { windowSize.x - 10, windowSize.y - 300 };
		constexpr ImVec2 contentPos = { Framework::ImguiHelpers::Centre(windowSize.x, contentSize.x), 50 };

		ImGui::SetNextWindowSize(contentSize);
		ImGui::SetNextWindowPos(contentPos);

		if (ImGui::Begin("Content", nullptr, windowFlags | ImGuiWindowFlags_NoCollapse))
		{
			Framework::ImguiHelpers::SetWindowFontSize(30.0f);

			if (ImGui::CollapsingHeader("Terrain", treeNodeFlags))
			{
				Framework::Data::Scope& terrain = mParamatersScope->GetScope("Terrain");
				{
					Framework::Data::Variable& numOfChunks = terrain.GetVariable("numOfChunks");
					glm::ivec2 ivec2Tmp;
					numOfChunks >> ivec2Tmp;
					if (ImGui::InputInt2("World size (chunks)", &ivec2Tmp[0]))
					{
						numOfChunks << glm::ivec2{ std::max(ivec2Tmp.x, 0), std::max(ivec2Tmp.y, 0) };
						mHasBeenSaved = false;
					}
				}
				{
					Framework::Data::Variable& seed = terrain.GetVariable("seed");
					int intTmp;
					seed >> intTmp;
					if (ImGui::InputInt("Seed", &intTmp))
					{
						seed << intTmp;
						mHasBeenSaved = false;
					}
				}
				{
					Framework::Data::Variable& maxHeight = terrain.GetVariable("maxHeight");
					float floatTmp;
					maxHeight >> floatTmp;
					if (ImGui::InputFloat("Max height", &floatTmp))
					{
						maxHeight << floatTmp;
						mHasBeenSaved = false;
					}
				}
				{
					Framework::Data::Variable& roughness = terrain.GetVariable("roughness");
					float floatTmp;
					roughness >> floatTmp;
					if (ImGui::InputFloat("Roughness", &floatTmp, 0.0f, 0.0f, "%.10f"))
					{
						roughness << floatTmp;
						mHasBeenSaved = false;
					}
				}
				{
					Framework::Data::Variable& persistence = terrain.GetVariable("persistence");
					float floatTmp;
					persistence >> floatTmp;
					if (ImGui::InputFloat("Persistence", &floatTmp, 0.0f, 0.0f, "%.10f"))
					{
						persistence << floatTmp;
						mHasBeenSaved = false;
					}
				}
				{
					Framework::Data::Variable& octaves = terrain.GetVariable("octaves");
					int intTmp;
					octaves >> intTmp;
					if (ImGui::InputInt("Octaves", &intTmp))
					{
						octaves << intTmp;
						mHasBeenSaved = false;
					}
				}
			}

			if (ImGui::CollapsingHeader("Forest", treeNodeFlags))
			{
				Framework::Data::Scope& forest = mParamatersScope->GetScope("Forest");
				{
					Framework::Data::Variable& enabled = forest.GetVariable("enabled");
					bool boolTmp;
					enabled >> boolTmp;
					if (ImGui::Checkbox("Enabled", &boolTmp))
					{
						enabled << boolTmp;
						mHasBeenSaved = false;
					}
				}
				{
					Framework::Data::Variable& forestSize = forest.GetVariable("forestSize");
					float floatTmp;
					forestSize >> floatTmp;
					if (ImGui::InputFloat("Forest size", &floatTmp))
					{
						forestSize << floatTmp;
						mHasBeenSaved = false;
					}
				}
				{
					Framework::Data::Variable& worldCoverage = forest.GetVariable("worldCoverage");
					float floatTmp;
					worldCoverage >> floatTmp;
					if (ImGui::InputFloat("World Coverage", &floatTmp))
					{
						worldCoverage << floatTmp;
						mHasBeenSaved = false;
					}
				}
				{
					Framework::Data::Variable& maxSteepness = forest.GetVariable("maxSteepness");
					float floatTmp;
					maxSteepness >> floatTmp;
					if (ImGui::InputFloat("Max steepness", &floatTmp))
					{
						maxSteepness << floatTmp;
						mHasBeenSaved = false;
					}
				}
			}

			ImGui::PushID(0);
			if (DrawArmyConfiguration(mParamatersScope->GetScope("Player")))
			{
				mHasBeenSaved = true;
			}
			ImGui::PopID();

			ImGui::PushID(1);
			if (DrawArmyConfiguration(mParamatersScope->GetScope("Opponent")))
			{
				mHasBeenSaved = true;
			}
			ImGui::PopID();

			{
				std::string savename = mParamatersScope->GetName();
				if (ImGui::InputText("Save name", &savename))
				{
					mParamatersScope->SetName(savename);
					mHasBeenSaved = false;
				}

				if (ImGui::Button("Generate random name"))
				{
					mParamatersScope->SetName(GenerateLevelName());
					mHasBeenSaved = false;
				}
			}
		}
		ImGui::End();

		ImGui::SetCursorPos({ contentPos.x, contentPos.y + contentSize.y + 10.0f });
		Framework::ImguiHelpers::SetWindowFontSize(50.0f);

		if (ImGui::Button("Save"))
		{
			std::string filepath = "levels/" + mParamatersScope->GetName() + ".txt";
			Save(filepath);
			mHasBeenSaved = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("Preview"))
		{
			Save("unsavedlevel.txt");
			std::unique_ptr<Sandbox> preview = std::make_unique<Sandbox>(mGame, "unsavedlevel.txt", mParamatersScope->GetName(), false, mHasBeenSaved);
			preview->mCollapseWindow = true;
			mGame.RequestLoadTo(std::move(preview));
		}

		ImGui::SameLine();
		if (ImGui::Button("Return to main menu"))
		{
			mGame.RequestLoadTo(std::make_unique<MainMenu>(mGame));
		}
	}
	ImGui::End();
}

void RTS::Sandbox::Save(const std::string& toFile) const
{
	Framework::Data::SavedData::MakeEmpty(toFile);
	Framework::Data::SavedData levelData = { toFile };
	
	Framework::Data::Scope& globalScope = levelData.GetScope();

	// Copy all the info
	globalScope.AddChild(mParamatersScope->GetName()).AddChild("LevelGeneration") = std::move(*mParamatersScope);
	levelData.Save();
}

std::string RTS::Sandbox::GenerateLevelName()
{
	uint seed = rand();
	std::string location = Framework::Random::GenerateRandomPronounceableString(Framework::Random::Range(5u, 10u), seed);
	location[0] = static_cast<char>(std::toupper(location[0]));
	return "The " + Framework::Random::RandomAdjective() + " battle of " + location;
}