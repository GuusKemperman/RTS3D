#include "precomp.h"
#include "Entity.h"

#include "TimeManager.h"
#include "Mesh.h"
#include "Scene.h"
#include "Camera.h"
#include "EntityManager.h"
#include "AssetManager.h"
#include "Scope.h"

Framework::Entity::Entity(Scene& scene) :
	mScene(scene),
	mTransform(this)
{
	mTimeSinceFixedTick = Random::Range(sFixedStepSize);
	mId = mScene.mEntityManager->AllocId(this);
}

Framework::Entity::~Entity()
{
	mScene.mEntityManager->FreeId(this, mId);
};

void Framework::Entity::Draw() const
{
	DrawOwnerAndChildren(mTransform, mTransform.GetLocalMatrix(), *mScene.mCamera);
}

void Framework::Entity::AttemptFixedTick()
{
	mTimeSinceFixedTick += TimeManager::GetDeltaTime();

	if (mTimeSinceFixedTick >= sFixedStepSize)
	{
		FixedTick();
		mTimeSinceFixedTick = fmodf(mTimeSinceFixedTick, sFixedStepSize);
	}
}

void DestroyTransformOwnerAndChildren(Framework::Transform& transform, Framework::EntityManager* entityManager)
{
	Framework::Entity* const transformOwner = transform.GetOwner();

	if (transformOwner != nullptr)
	{
		entityManager->RemoveEntity(transformOwner->GetId());
	}

	for (Framework::Transform* child : transform.GetChildren())
	{
		DestroyTransformOwnerAndChildren(*child, entityManager);
	}
}

void Framework::Entity::Destroy()
{
	DestroyTransformOwnerAndChildren(GetTransform(), mScene.mEntityManager.get());
}

bool Framework::Entity::Serialize(Data::Scope& parentScope) const
{
	Data::Scope& myScope = parentScope.AddChild("Entity");
	myScope.AddVariable("id") << mId;

	mTransform.Serialize(myScope);

	const btRigidBody* myRigidBody = dynamic_cast<const btRigidBody*>(mCollisionObject.get());

	if (myRigidBody != nullptr)
	{
		Data::Scope& rigidBodyScope = parentScope.AddChild("RigidBody");
		rigidBodyScope.AddVariable("lVel") << Math::ToGLM(myRigidBody->getLinearVelocity());
		rigidBodyScope.AddVariable("aVel") << Math::ToGLM(myRigidBody->getAngularVelocity());
		rigidBodyScope.AddVariable("force") << Math::ToGLM(myRigidBody->getTotalForce());
	}

	return true;
}

void Framework::Entity::Deserialize(const Data::Scope& parentScope)
{
	const Data::Scope& myScope = parentScope.GetScope("Entity");

	mScene.mEntityManager->FreeId(this, mId);
	EntityId tmpId;
	myScope.GetVariable("id") >> tmpId;
	mId = mScene.mEntityManager->AllocId(this, tmpId);

	mTransform.Deserialize(myScope, mScene);

	if (mCollisionObject != nullptr)
	{
		mCollisionObject->activate();
		mCollisionObject->setWorldTransform(mTransform.ToBullet());

		btRigidBody* myRigidBody = dynamic_cast<btRigidBody*>(mCollisionObject.get());
		if (myRigidBody != nullptr)
		{
			myRigidBody->clearForces();
			myRigidBody->clearGravity();

			const Data::Scope& rigidBodyScope = parentScope.GetScope("RigidBody");

			glm::vec3 lVel, aVel, force;
			rigidBodyScope.GetVariable("lVel") >> lVel;
			rigidBodyScope.GetVariable("aVel") >> aVel;
			rigidBodyScope.GetVariable("force") >> force;

			myRigidBody->setLinearVelocity(Math::ToBullet(lVel));
			myRigidBody->setAngularVelocity(Math::ToBullet(aVel));
			myRigidBody->applyCentralForce(Math::ToBullet(force));
		}
	}
}

void Framework::Entity::DrawOwnerAndChildren(const Transform& transform, const glm::mat4& worldMatrix, Camera& camera)
{
	Entity* owner = transform.GetOwner();

	if (owner != nullptr)
	{
		std::optional<MeshId> meshId = owner->GetMeshId();
		
		if (meshId.has_value())
		{
			camera.RequestInstanceDraw(meshId.value(), worldMatrix);
		}
	}

	for (const Transform* child : transform.GetChildren())
	{
		DrawOwnerAndChildren(*child, worldMatrix * child->GetLocalMatrix(), camera);
	}
}