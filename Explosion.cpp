#include "precomp.h"
#include "Explosion.h"

#include "Inquirer.h"
#include "Scene.h"
#include "Physics.h"
#include "Unit.h"
#include "AnimatedMesh.h"
#include "Animator.h"
#include "Animation.h"
#include "TimeManager.h"
#include "AssetManager.h"
#include "Scope.h"

RTS::Explosion::Explosion(Framework::Scene& scene, const Framework::Transform& transform, const float explosionForce) :
	Entity(scene),
	mCollisionShape(explosionForce)
{
	Framework::Transform& myTransform = GetTransform();

	mMesh = Framework::AssetManager::Inst().GetAsset<Framework::AnimatedMesh>("models/explosion.dae");
	mAnimator = std::make_unique<Framework::Animator>(mMesh->GetAnimation(0));
	mAnimator->mLoopAnimation = false;

	if (explosionForce != 0.0f)
	{
		myTransform = transform;

		const float explosionRadius = explosionForce * .25f;
		const float meshRadius = explosionRadius * .8f;

		mGrowSpeed *= 1.0f / meshRadius;

		myTransform.SetLocalScale(glm::vec3{ meshRadius });
		myTransform.SetLocalOrientation(Framework::Random::Range(-PI, PI), Framework::Random::Range(-PI, PI), Framework::Random::Range(-PI, PI));
		ApplyForcesAndDamage(explosionRadius, explosionForce);
	}

	mCollisionObject = std::make_unique<btCollisionObject>();
	mCollisionObject->setCollisionShape(&mCollisionShape);
	mCollisionObject->setWorldTransform(myTransform.ToBullet());
	mCollisionObject->setUserPointer(this);

	mScene.mPhysics->AddCollisionObjectToWorld(mCollisionObject.get(), Framework::Physics::Group::visibileButNoCollisionGroup, Framework::Physics::Mask::visibleButNoCollisionMask);
}

RTS::Explosion::~Explosion()
{
	mScene.mPhysics->RemoveCollisionObjectFromWorld(std::move(mCollisionObject));
}

void RTS::Explosion::Tick()
{
	mAnimator->UpdateAnimation(mGrowSpeed * Framework::TimeManager::GetDeltaTime());

	if (mAnimator->mCurrentTime >= mAnimator->GetAnimation()->GetDuration())
	{
		Destroy();
	}
}

void RTS::Explosion::Draw() const
{
	mMesh->Draw(*mScene.mCamera, GetTransform().GetLocalMatrix(), mAnimator.get());
}

bool RTS::Explosion::Serialize(Framework::Data::Scope& parentScope) const
{
	Entity::Serialize(parentScope);

	Framework::Data::Scope& myScope = parentScope.AddChild("Explosion");
	myScope.AddVariable("growSpeed") << mGrowSpeed;
	myScope.AddVariable("animTime") << mAnimator->mCurrentTime;

	return true;
}

void RTS::Explosion::Deserialize(const Framework::Data::Scope& parentScope)
{
	Entity::Deserialize(parentScope);

	const Framework::Data::Scope& myScope = parentScope.GetScope("Explosion");
	myScope.GetVariable("growSpeed") >> mGrowSpeed;
	myScope.GetVariable("animTime") >> mAnimator->mCurrentTime;
}

void RTS::Explosion::ApplyForcesAndDamage(const float explosionRadius, const float explosionForce)
{
	Framework::Inquirer inquirer{};
	inquirer.mCollisionObject.setCollisionShape(&mCollisionShape);
	inquirer.m_collisionFilterGroup = Framework::Physics::Group::projectileGroup;
	inquirer.m_collisionFilterMask = Framework::Physics::Mask::projectileMask;

	const Framework::Transform& myTransform = GetTransform();
	mScene.mPhysics->Query(inquirer, myTransform);

	const glm::vec3 myPosition = myTransform.GetLocalPosition();
	for (const btCollisionObject* obj : inquirer.mCollidedWith)
	{
		// Removing const qualifier, may break things, but lets try it out.
		btRigidBody* rigidBody = const_cast<btRigidBody*>(dynamic_cast<const btRigidBody*>(obj));

		if (rigidBody == nullptr)
		{
			continue;
		}

		const glm::vec3 deltaPosition = Framework::Math::ToGLM(rigidBody->getWorldTransform().getOrigin()) - myPosition;
		float forceScalar = explosionForce;
		const float distance = glm::length(deltaPosition);

		if (distance != 0.0f)
		{
			forceScalar *= std::max(1.0f - (distance / explosionRadius), 0.0f);

			const glm::vec3 normalizedDelta = deltaPosition / distance;

			rigidBody->applyImpulse(Framework::Math::ToBullet(normalizedDelta * forceScalar), Framework::Math::ToBullet(-deltaPosition));
			rigidBody->applyTorqueImpulse(Framework::Math::ToBullet(normalizedDelta * forceScalar * .05f));
		}

		Unit* const unit = dynamic_cast<Unit*>(static_cast<Entity*>(rigidBody->getUserPointer()));
		if (unit != nullptr)
		{
			unit->ReceiveDamage(forceScalar);
		}
	}
}