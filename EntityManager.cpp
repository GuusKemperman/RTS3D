#include "precomp.h"
#include "EntityManager.h"

#include <typeinfo>
#include <typeindex>

#include "Scope.h"

#include "Scene.h"

Framework::EntityManager::EntityManager(Scene& scene) :
	mScene(scene)
{

}

Framework::EntityManager::~EntityManager()
{
	Clear();
}

void Framework::EntityManager::Tick()
{
	while (!mIdRequests.empty())
	{
		const IdRequest& request = mIdRequests.front();
		
		Entity* entity = GetEntity(request.mId);
		request.mCallback(*entity, request.mSecondParam);
		mIdRequests.pop();
	}

	for (uint i = 0; i < mEntities.size(); i++)
	{
		Entity* entity = mEntities[i].get();

		if (entity->HasFixedTick())
		{
			entity->AttemptFixedTick();
		}

		entity->Tick();
	}
}

void Framework::EntityManager::DrawEntities() const
{
	for (const std::unique_ptr<Entity>& entity : mEntities)
	{
		if (entity->GetTransform().IsOrphan())
		{
			entity->Draw();
		}
	}
}

void Framework::EntityManager::RemoveEntity(EntityId id)
{
#ifdef DEBUG
	if (auto it = std::find(mToRemoveAsVector.begin(), mToRemoveAsVector.end(), id); it != mToRemoveAsVector.end())
	{
		LOGWARNING("Removing entity " << id << " twice");
		return;
	}
	mToRemoveAsVector.push_back(id);
#endif // DEBUG
	mToRemove.push(id);
}

Framework::Entity* Framework::EntityManager::GetEntity(const EntityId id) const
{
	return mEntityLookUp.at(id);
}

std::optional<Framework::Entity*> Framework::EntityManager::TryGetEntity(const EntityId id) const
{
	auto it = mEntityLookUp.find(id);

	if (it != mEntityLookUp.end())
	{
		return std::optional<Entity*>(it->second);
	}

	static std::optional<Entity*> emptyOptional{};
	return emptyOptional;
}

Framework::EntityId Framework::EntityManager::GenerateEntityId()
{
	EntityId id;
	do
	{
		id = mNextIdToGive++;
	} while (IsIdTaken(id));

	return id;
}

Framework::Entity::EntityIdWrapper Framework::EntityManager::AllocId(Entity* entity, EntityId id)
{
	assert(!IsIdTaken(id));
	mEntityLookUp[id] = entity;
	return id;
}

void Framework::EntityManager::FreeId(const Entity* currentOwner, EntityId id)
{
	if (currentOwner == nullptr)
	{
		return;
	}
	assert(mEntityLookUp.at(id) == currentOwner);
	mEntityLookUp.erase(id);
}

void Framework::EntityManager::DeconstructDestroyedEntities()
{
	while (!mToRemove.empty())
	{
		EntityId id = mToRemove.front();
		mToRemove.pop();

		auto it = mEntityLookUp.find(id);
		Entity* entity = it->second;

		auto itToErase = find_if(mEntities.begin(), mEntities.end(),
			[entity](const std::unique_ptr<Entity>& a)
			{
				return a.get() == entity;
			});
		size_t index = itToErase - mEntities.begin();

		mEntities[index] = move(mEntities.back());
		mEntities.pop_back();
	}

#ifdef DEBUG
	mToRemoveAsVector.clear();
#endif // DEBUG
}

void Framework::EntityManager::RemoveInvalidIds(std::vector<EntityId>& ids) const
{
	for (size_t i = 0; i < ids.size();)
	{
		EntityId& id = ids[i];

		if (!TryGetEntity(id).has_value())
		{
			id = ids.back();
			ids.pop_back();
		}
		else
		{
			++i;
		}
	}
}

void Framework::EntityManager::Serialize(Framework::Data::Scope& parentScope) const
{
	Data::Scope& myScope = parentScope.AddChild("EntityManager");

	for (const std::unique_ptr<Entity>& entity : mEntities)
	{
		const char* typeName = entity->GetTypeIndex().name();
		if (sFactories.find(typeName) == sFactories.end())
		{
			// We can't deserialize this, so don't bother serializing it.
			continue;
		}

		Data::Scope& entityScope = myScope.AddChild(typeName);

		bool neededSerialization = entity->Serialize(entityScope);

		if (!neededSerialization)
		{
			LOGWARNING("Do we really need a factory for " << typeName << "?");
			// Because we're not serializing this, we should remove our assigned parent scope to make the save size smaller.
			myScope.RemoveScope(typeName);
		}
	}
}

float Framework::EntityManager::Deserialize(const Framework::Data::Scope& parentScope, const EntityId maxNumOfToDeserialze)
{
	std::optional<const Data::Scope*> myOptionalScope = parentScope.TryGetScope("EntityManager");
	if (!myOptionalScope.has_value())
	{
		LOGWARNING("Could not load in entityManager from file, data missing");
		return 1.0f;
	}


	const Data::Scope& myScope = *myOptionalScope.value();
	const std::vector<Data::Scope>& savedEntities = myScope.GetChildren();
	
	EntityId amountDeserializedThisCycle{};

	for (EntityId i = mAmountDeserialized; i < savedEntities.size() && amountDeserializedThisCycle < maxNumOfToDeserialze; i++, amountDeserializedThisCycle++)
	{
		const Data::Scope& entityScope = savedEntities[i];
		const std::string& type = entityScope.GetName();

		std::unique_ptr<Entity> entity = sFactories.at(type)->Create(mScene);
		entity->Deserialize(entityScope);

		// Saves time on assigned new ids, since it has to go over less duplicates
		mNextIdToGive = std::max(mNextIdToGive, static_cast<EntityId>(entity->GetId() + 1u));

		AddEntity(std::move(entity));
	}
	mAmountDeserialized += amountDeserializedThisCycle;

	const float percentageGenerated = static_cast<float>(mAmountDeserialized) / static_cast<float>(savedEntities.size());
	return percentageGenerated;
}

void Framework::EntityManager::DelayedIdRequest(const EntityId id, std::function<void(Entity&, void*)> callback, void* secondParam)
{
	mIdRequests.emplace(id, std::move(callback), secondParam);
}

void Framework::EntityManager::Clear()
{
	mEntities.clear();

	assert(mEntityLookUp.empty()
		&& "There are entity id's that did not get freed!");
}
