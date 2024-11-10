#pragma once
#include "Entity.h"

namespace Framework
{
	namespace Data
	{
		class Scope;
	}

	class Scene;

	class EntityManager
	{
	public:
		EntityManager(Scene& scene);
		~EntityManager();

		EntityManager(EntityManager&) = delete;

		void Tick();
		void DrawEntities() const;

		template<typename T, typename ...Args>
		inline T& AddEntity(Args && ...args);
		
		template<typename T>
		inline T& AddEntity(std::unique_ptr<T> entity);

		void RemoveEntity(EntityId id);

		Entity* GetEntity(const EntityId id) const;
		std::optional<Entity*> TryGetEntity(const EntityId id) const;

		EntityId GenerateEntityId();
		bool IsIdTaken(const EntityId id) { return TryGetEntity(id).has_value(); }

		[[nodiscard]] inline Entity::EntityIdWrapper AllocId(Entity* entity) { return AllocId(entity, GenerateEntityId()); }
		[[nodiscard]] Entity::EntityIdWrapper AllocId(Entity* entity, EntityId id);
		void FreeId(const Entity* currentOwner, EntityId id);

		void DeconstructDestroyedEntities();

		// Does not preserve order
		void RemoveInvalidIds(std::vector<EntityId>& ids) const;

		template<typename DerivedFromEntity>
		static inline std::vector<EntityId> ConvertToEntityIds(const std::vector<DerivedFromEntity*>& entities);

		// Does not check to see if the unit that the id belongs to is actually of that type, it just does a static cast.
		template<typename T>
		inline std::vector<T*> ConvertToType(const std::vector<EntityId>& ids) const;

		template<typename OfType>
		inline std::vector<OfType*> GetEntities() const;

		void Serialize(Framework::Data::Scope& parentScope) const;
		float Deserialize(const Framework::Data::Scope& parentScope, const EntityId maxNumOfToDeserialze = std::numeric_limits<EntityId>::max());

		// Will wait with looking for the id until the start of the next frame.
		void DelayedIdRequest(const EntityId id, std::function<void(Entity&, void*)> callback, void* secondParam = nullptr);

		template<typename T>
		static void BuildFactory();

		void Clear();

	private:
		Scene& mScene;

		std::vector<std::unique_ptr<Entity>> mEntities{};
		std::unordered_map<EntityId, Entity*> mEntityLookUp{};

		std::queue<EntityId> mToRemove{};

		EntityId mNextIdToGive = 1;

#ifdef DEBUG
		// Only used to check if an entity has already requested to be removed.
		std::vector<EntityId> mToRemoveAsVector{};
#endif // DEBUG

		struct IdRequest
		{
			IdRequest(EntityId id, std::function<void(Entity&, void*)> callBack, void* secondParm) : mId(id), mCallback(std::move(callBack)), mSecondParam(secondParm) {}
			const EntityId mId{};
			const std::function<void(Entity&, void*)> mCallback{};
			void* const mSecondParam{};
		};
		std::queue<IdRequest> mIdRequests{};

		// Needed for serialization
		static inline std::unordered_map<std::string, std::unique_ptr<Entity::FactoryBase>> sFactories{};
		EntityId mAmountDeserialized{};
	};

	template<typename T, typename ...Args>
	inline T& EntityManager::AddEntity(Args && ...args)
	{
		return AddEntity(std::make_unique<T>(mScene, std::forward<Args>(args)...));
	}

	template<typename T>
	inline T& EntityManager::AddEntity(std::unique_ptr<T> entity)
	{
		T* rawPtr = entity.get();
		mEntities.push_back(std::move(entity));
		return *rawPtr;
	}

	template<typename DerivedFromEntity>
	inline std::vector<EntityId> EntityManager::ConvertToEntityIds(const std::vector<DerivedFromEntity*>& entities)
	{
		std::vector<EntityId> ids(entities.size());

		for (size_t i = 0; i < entities.size(); i++)
		{
			ids[i] = static_cast<Entity*>(entities[i])->GetId();
		}

		return ids;
	}

	template<typename T>
	inline std::vector<T*> EntityManager::ConvertToType(const std::vector<EntityId>& ids) const
	{
		std::vector<T*> ofType(ids.size());

		for (size_t i = 0; i < ids.size(); i++)
		{
			ofType[i] = static_cast<T*>(GetEntity(ids[i]));
		}

		return ofType;
	}

	template<typename OfType>
	inline std::vector<OfType*> EntityManager::GetEntities() const
	{
		std::vector<OfType*> found{};
		found.reserve(mEntities.size());

		for (const std::unique_ptr<Entity>& entity : mEntities)
		{
			OfType* asType = dynamic_cast<OfType*>(entity.get());

			if (asType != nullptr)
			{
				found.push_back(asType);
			}
		}
		return found;
	}

	template<typename T>
	inline void EntityManager::BuildFactory()
	{
		std::unique_ptr<Entity::FactoryBase> factory = std::make_unique<T>();
		const std::string& forType = factory->GetTypeIndex().name();
		sFactories[forType] = std::move(factory);
	}
}