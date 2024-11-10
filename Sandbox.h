#pragma once
#include "Level.h"

namespace Framework::Data 
{
	class Scope;
}

namespace RTS
{
	class Army;
	class Player;
	class ProceduralUnitFactory;

	class Sandbox :
		public Level
	{
	public:
		Sandbox(Framework::Game& game, const std::string& levelFile, const std::string& levelName, const bool generateNewLevelName = false, const bool hasBeenSaved = true);
		~Sandbox();

		uchar Deserialize(const uchar progress) override;
		void Tick() override;
		void DrawImGui() override;

	private:
		void Save(const std::string& toFile) const;
		static std::string GenerateLevelName();
		bool DrawArmyConfiguration(Framework::Data::Scope& owner);

		Player* mPlayer{};

		std::unique_ptr<Framework::Data::Scope> mParamatersScope{};
		bool mHasBeenSaved{};
		bool mCollapseWindow{};

		std::shared_ptr<ProceduralUnitFactory> mProceduralUnitFactory{};
	};
}