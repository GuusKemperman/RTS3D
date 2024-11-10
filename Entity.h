#pragma once
#include "Transform.h"
#include <typeindex>

#define ENTITYMAKER(className)																															\
public:																																					\
	struct Factory final :																																\
		public Framework::Entity::FactoryBase																											\
			{																																			\
				inline std::unique_ptr<Framework::Entity> Create(Framework::Scene& scene) const override { return std::make_unique<className>(scene); }	\
				inline std::type_index GetTypeIndex() const override { return typeid(className); }														\
			};																																			\
		inline virtual std::type_index GetTypeIndex() const { return typeid(className); }																\
private:																																				\

namespace Framework
{
	namespace Data
	{
		class Scope;
	}

	class Scene;
	class Mesh;
	class EntityManager;
	class Camera;

	class Entity
	{
	public:
		struct FactoryBase
		{
			FactoryBase() = default;
			virtual ~FactoryBase() = default;
			virtual std::unique_ptr<Entity> Create(Scene& scene) const = 0;
			virtual std::type_index GetTypeIndex() const = 0;
		};
		ENTITYMAKER(Entity)
	public:
		Entity(Scene& scene);
		virtual ~Entity();

		const Transform& GetTransform() const { return mTransform; }
		Transform& GetTransform() { return mTransform; }

		class EntityIdWrapper
		{
		public:
			EntityIdWrapper() = default;
			EntityIdWrapper(EntityId id) : mId(id) {};
			operator const EntityId& () const { return mId; }
		private:
			friend class EntityManager;
			EntityId mId{};
		};
		const EntityIdWrapper& GetId() const { return mId; }
		
		virtual void Tick() {};
		virtual void FixedTick() {};

		virtual void Draw() const;

		void AttemptFixedTick();
		inline bool HasFixedTick() const { return mHasFixedTick; }

		virtual void OnCollision(const btCollisionObject*) {};
		inline bool HasCollisionCallback() const { return mHasCollisionCallback; }

		void Destroy();

		// Returns false if this entity does not need to be serialized
		virtual bool Serialize(Data::Scope& parentScope) const;
		virtual void Deserialize(const Data::Scope& parentScope);

		inline btCollisionObject* GetCollisionObject() const { return mCollisionObject.get(); }
		inline const std::optional<MeshId>& GetMeshId() const { return mMeshId; }

		inline Scene& GetScene() const { return mScene; }

	protected:
		static void DrawOwnerAndChildren(const Transform& transform, const glm::mat4& worldMatrix, Camera& camera);

		static constexpr float sFixedStepSize = 0.2f;
		bool mHasFixedTick{};
		bool mHasCollisionCallback{};

		std::optional<MeshId> mMeshId{};

		Scene& mScene;

		std::unique_ptr<btCollisionObject> mCollisionObject{};

	private:
		EntityIdWrapper mId;
		Transform mTransform{};

		float mTimeSinceFixedTick{};
	};
}