#pragma once
#include "Level.h"

namespace Framework
{
    class ImGuiFontWrapper;
}

namespace RTS
{
    class Army;
    
    class MainMenu :
        public Level
    {
    public:
        MainMenu(Framework::Game& game);
        ~MainMenu();

        uchar Deserialize(const uchar progress) override;
        void Tick() override;
        void DrawImGui() override;

    private:
        void ReplenishArmies() const;

        glm::vec2 RandomCameraPosition() const;

        static constexpr uint sDesiredArmySize = 500u;

        static constexpr float sMinCamDistFromEdges = 100.0f;
        static constexpr float sTimeBetweenCamDirChange = 10.0f;
        static constexpr float sCamMoveSpeed = 10.0f;

        glm::vec2 mCamDir{};
        float mTimeCamDirChanged = -sTimeBetweenCamDirChange;

        enum class Menu : uchar { main, level, saves, settings };
        Menu mActiveMenu = Menu::main;

        // Needed for main menu
        std::shared_ptr<Framework::ImGuiFontWrapper> mMainTitleFont{};

        // Needed for level selection
        std::vector<std::string> mLevelNames{};
        int mSelectedLevel = -1;

        // Needed for save selection
        std::vector<std::string> mSavesNames{};
        int mSelectedSave = -1;

        std::unique_ptr<Framework::Data::Scope> mNewSettings{};
    };
}