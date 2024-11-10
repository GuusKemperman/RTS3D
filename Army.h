#pragma once
#include "Entity.h"
#include "Scene.h"
#include "EntityManager.h"

namespace Framework
{
	class Scene;

	namespace Data
	{
		class Scope;
	}
}

namespace RTS
{
	class Unit;
	class ProceduralUnitFactory;

	class Army :
		public Framework::Entity
	{
		ENTITYMAKER(Army)
	public:
		Army(Framework::Scene& scene, ArmyId id = ArmyId::null);

		void SpawnUnits(const Framework::Data::Scope& loadFrom);

		const std::vector<Framework::EntityId>& GetIds();
		std::vector<Unit*> GetUnitsInArmy();

		template<typename T, typename ...Args>
		inline T& AddToArmy(Args && ...args);

		inline ArmyId GetArmyId() const { return mArmyId; }

		bool Serialize(Framework::Data::Scope& parentScope) const override;
		void Deserialize(const Framework::Data::Scope& parentScope) override;

		size_t Size();
		inline size_t NumOfUnitsSpawned() const { return mNumOfUnitsSpawned; }

	private:
		void RemoveInvalidIds();

		std::shared_ptr<ProceduralUnitFactory> mProceduralUnitFactory{};

		ArmyId mArmyId{};
		std::vector<Framework::EntityId> mUnits{};
		size_t mNumOfUnitsSpawned{};
	};

	template<typename T, typename ...Args>
	inline T& Army::AddToArmy(Args && ...args)
	{
		static_assert(std::is_base_of<Unit, T>::value, "T must inherit from unit");
		T& newEntity = mScene.mEntityManager->AddEntity<T>(this, std::forward<Args>(args)...);
		mUnits.push_back(newEntity.GetId());
		++mNumOfUnitsSpawned;
		return newEntity;
	}
}