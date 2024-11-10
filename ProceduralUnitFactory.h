#pragma once
#include "Transform.h"

namespace Framework
{
	class EntityManager;
}

namespace RTS
{
	class Unit;

	struct UnitBodyData
	{
		UnitBodyData(const std::string& filePath);

		inline Framework::MeshId GetMeshId(ArmyId armyId) const { return mMeshesForEachArmy.at(static_cast<size_t>(armyId) - 1); }

		std::string mName{};
		size_t mIndexInFactory{};
		std::array<Framework::MeshId, 2> mMeshesForEachArmy{};
		std::unique_ptr<btBoxShape> mShape{};
		btVector3 mInertia{};

		std::vector<Framework::Transform> mPossibleTurretNodes{};

		float mHoverAtHeight{};
		float mMaxHealth{};
		float mMovementSpeed{};
		float mTurnSpeed{};
		float mMass{};
		float mDeathExplosionSize{};
	};

	struct TurretData
	{
		TurretData(const std::string& filePath);

		std::string mName{};
		Framework::MeshId mMeshId{};

		std::unique_ptr<btCollisionShape> mCanFireWithinShape{};
		size_t mIndexInFactory{};
		float mCooldown{};
		float mTurnSpeed{};
		float mDamage{};
	};

	class ProceduralUnitFactory
	{
	public:
		ProceduralUnitFactory(const std::string& directory);

		using CumulativeFrequency = std::pair<size_t, float>;
		static size_t RandomIndex(const std::vector<CumulativeFrequency>& frequencies);

		void AssignRandomParts(Unit& unit, Framework::EntityManager& whatToAddTheTurretsTo, 
			const std::vector<CumulativeFrequency>& bodyCumulaitiveFrequencies, const std::vector<CumulativeFrequency>& turretCumulaitiveFrequencies) const;

		inline const UnitBodyData* GetBody(size_t index) const { return &mBodyData[index]; }
		inline const TurretData* GetTurret(size_t index) const { return &mTurretData[index]; }

		const UnitBodyData* GetBody(const std::string& name) const;
		const TurretData* GetTurret(const std::string& name) const;

		inline const UnitBodyData* RandomBody() const { return &mBodyData[rand() % mBodyData.size()]; }
		inline const TurretData* RandomTurret() const { return &mTurretData[rand() % mTurretData.size()]; }

		inline const std::vector<UnitBodyData>& GetBodies() const { return mBodyData; }
		inline const std::vector<TurretData>& GetTurrets() const { return mTurretData; }

	private:
		std::vector<UnitBodyData> mBodyData{};
		std::vector<TurretData> mTurretData{};
	};
}