#pragma once
#include "Entity.h"
#include "Inquirer.h"

namespace RTS
{
	class Unit;
	struct TurretData;

	class Turret :
		public Framework::Entity
	{
		ENTITYMAKER(Turret);
	public:
		Turret(Framework::Scene& scene, const TurretData* turretData = nullptr, Unit* attachTo = nullptr, std::optional<Framework::Transform> nodeOnUnitBody = {});

		void Tick() override;
		void FixedTick() override;

		bool Serialize(Framework::Data::Scope& parentScope) const;

		void Deserialize(const Framework::Data::Scope& parentScope);

	private:
		void SetTurretData(const TurretData* turretData);

		void LockOnto(Framework::EntityId id);
		bool IsInSights(const Entity* entity) const;

		const TurretData* mTurretData{};
		Framework::Transform mAttachedToNode{};
		Framework::Inquirer mInquirer{};

		std::optional<Framework::EntityId> mDesiredTarget{};
		std::optional<Framework::EntityId> mLockedOntoTarget{};

		float mTimeLockStart = -INFINITY;
		float mTimeLastFired = -INFINITY;
	};
}