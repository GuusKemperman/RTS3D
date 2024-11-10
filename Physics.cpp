#include "precomp.h"
#include "Physics.h"

#include "Entity.h"
#include "Scene.h"
#include "Camera.h"
#include "Inquirer.h"
#include "Transform.h"
#include "TimeManager.h"
#include "Terrain.h"

Framework::Physics::Physics(Scene& scene) :
	mScene(scene),
	mDebugDrawer(scene)
{
	mBroadPhase = new btDbvtBroadphase;
	mCollisionConfiguration = new btDefaultCollisionConfiguration;
	mDispatcher = new btCollisionDispatcher(mCollisionConfiguration);
	mConstraintSolver = new btSequentialImpulseConstraintSolver;
	mWorld = new btDiscreteDynamicsWorld(mDispatcher, mBroadPhase, mConstraintSolver, mCollisionConfiguration);
	
	mWorld->setDebugDrawer(&mDebugDrawer);
	mWorld->setGravity({ 0.0f, -9.81f, 0.0f });
}

Framework::Physics::~Physics()
{
	assert(mWorld->getNumCollisionObjects() == 0 && "Some object's have been added but never removed!");
	
	delete mWorld;
	delete mConstraintSolver;
	delete mDispatcher;
	delete mCollisionConfiguration;
	delete mBroadPhase;
}

void Framework::Physics::Tick()
{
	mWorld->stepSimulation(TimeManager::GetDeltaTime());

	int numOfManifolds = mDispatcher->getNumManifolds();
	for (int i = 0; i < numOfManifolds; ++i)
	{
		btPersistentManifold* manifold = mDispatcher->getManifoldByIndexInternal(i);
		if (manifold->getNumContacts() > 0)
		{
			const btCollisionObject* objA = manifold->getBody0();
			const btCollisionObject* objB = manifold->getBody1();

			Entity* ownerA = static_cast<Entity*>(objA->getUserPointer());
			Entity* ownerB = static_cast<Entity*>(objB->getUserPointer());

			if (ownerA != nullptr
				&& ownerA->HasCollisionCallback())
			{
				static_cast<Entity*>(ownerA)->OnCollision(objB);
			}

			if (ownerB != nullptr
				&& ownerB->HasCollisionCallback())
			{
				static_cast<Entity*>(ownerB)->OnCollision(objA);
			}
		}
	}
}

void Framework::Physics::DebugDraw()
{
	mWorld->debugDrawWorld();
}

void Framework::Physics::AddCollisionObjectToWorld(btCollisionObject* object, Group group, Mask mask)
{
	if (object->getInternalType() == btCollisionObject::CO_RIGID_BODY)
	{
		mWorld->addRigidBody(static_cast<btRigidBody*>(object), static_cast<int>(group), static_cast<int>(mask));
	}
	else
	{
		mWorld->addCollisionObject(object, static_cast<int>(group), static_cast<int>(mask));
	}
}

void Framework::Physics::RemoveCollisionObjectFromWorld(std::unique_ptr<btCollisionObject> object)
{
	mWorld->removeCollisionObject(object.get());
}

void Framework::Physics::Query(Inquirer& inquirer, const Transform& transform) const
{
	inquirer.mCollidedWith.clear();

	btTransform bulletTransform = transform.ToBullet();
	inquirer.mCollisionObject.setWorldTransform(bulletTransform);
	mWorld->contactTest(&inquirer.mCollisionObject, inquirer);

	if (mDebugDrawer.getDebugMode() != btIDebugDraw::DBG_NoDebug)
	{
		mWorld->debugDrawObject(bulletTransform, inquirer.mCollisionObject.getCollisionShape(), { 0.0f, 0.0f, 1.0f });
	}
}

// https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=3976 Borrowed from here
//std::vector<const btCollisionObject*> Framework::Physics::GetNarrowPhaseCollisions(btPairCachingGhostObject& forObject) const
//{
//	btOverlappingPairCache* pairCache = forObject.getOverlappingPairCache();
//
//	mDispatcher->dispatchAllCollisionPairs(pairCache, mWorld->getDispatchInfo(), mDispatcher);
//
//	btBroadphasePairArray& collisionPairs = pairCache->getOverlappingPairArray();
//
//	constexpr bool onlyNegativePenetration = true;
//
//	const int numObjects = collisionPairs.size();
//	static btManifoldArray	m_manifoldArray;
//	bool added;
//
//	std::vector<const btCollisionObject*> objectsFound{};
//	objectsFound.reserve(numObjects);
//
//	for (int i = 0; i < numObjects; i++)
//	{
//		const btBroadphasePair& collisionPair = collisionPairs[i];
//		m_manifoldArray.resize(0);
//		if (collisionPair.m_algorithm) collisionPair.m_algorithm->getAllContactManifolds(m_manifoldArray);
//		else printf("No collisionPair.m_algorithm - probably m_dynamicsWorld->getDispatcher()->dispatchAllCollisionPairs(...) must be missing.\n");
//		added = false;
//		for (int j = 0; j < m_manifoldArray.size(); j++)
//		{
//			btPersistentManifold* manifold = m_manifoldArray[j];
//			// Here we are in the narrowphase, but can happen that manifold->getNumContacts()==0:
//			if (onlyNegativePenetration)
//			{
//				for (int p = 0; p < manifold->getNumContacts(); p++)
//				{
//					const btManifoldPoint& pt = manifold->getContactPoint(p);
//					if (pt.getDistance() < 0.0)
//					{
//						// How can I be sure that the colObjs are all distinct ? I use the "added" flag.
//						objectsFound.push_back(manifold->getBody0() == &forObject ? manifold->getBody1() : manifold->getBody0());
//						added = true;
//						break;
//					}
//				}
//				if (added) break;
//			}
//			else if (manifold->getNumContacts() > 0)
//			{
//				objectsFound.push_back(manifold->getBody0() == &forObject ? manifold->getBody1() : manifold->getBody0());
//				break;
//			}
//		}
//	}
//	return objectsFound;
//}

Framework::Physics::RayCastHit Framework::Physics::RayCast(const glm::vec3 start, glm::vec3 direction, float maxDistance) const
{
	const TerrainData* const terrainData = mScene.mTerrain->GetData();

	direction = normalize(direction);

	constexpr uint numOfSamples = 1048u;
	float maxStepSize = 5.0f;
	constexpr float marginOfError = 0.05f;

	// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection
	float tmin = -start.x / direction.x;
	float tmax = (terrainData->mWorldSizeX - start.x) / direction.x;

	if (tmin > tmax)
	{
		std::swap(tmin, tmax);
	}

	float tymin = -start.y / direction.y;
	float tymax = -INFINITY;

	if (tymin > tymax)
	{
		std::swap(tymin, tymax);
	}

	if ((tmin > tymax) || (tymin > tmax))
	{
		return RayCastHit();
	}

	if (tymin > tmin)
	{
		tmin = tymin;
	}

	if (tymax < tmax)
	{
		tmax = tymax;
	}

	float tzmin = -start.z / direction.z;
	float tzmax = (terrainData->mWorldSizeZ - start.z) / direction.z;

	if (tzmin > tzmax)
	{
		std::swap(tzmin, tzmax);
	}

	if ((tmin > tzmax) || (tzmin > tmax))
	{
		return RayCastHit();
	}

	if (tzmin > tmin)
	{
		tmin = tzmin;
	}

	if (tzmax < tmax)
	{
		tmax = tzmax;
	}

	float hitMin = tmin;
	float hitMax = std::min(tmax, maxDistance);

	if (isnan(hitMin) || isnan(hitMax))
	{
		return RayCastHit();
	}

	for (uint i = 0; i < numOfSamples; i++)
	{
		const float travelDistance = std::min(hitMin + maxStepSize, Math::lerp(hitMin, hitMax, .5f));
		const glm::vec3 sampleAt = start + direction * travelDistance;

		const float terrainHeight = terrainData->GetHeightAtPosition(sampleAt.x, sampleAt.z);

		if (terrainHeight >= sampleAt.y) // Hit terrain
		{
			hitMax = std::min(travelDistance, hitMax);
		}
		else if (terrainHeight < sampleAt.y)
		{
			hitMin = std::max(travelDistance, hitMin);
		}

		if (abs(terrainHeight - sampleAt.y) <= marginOfError)
		{
			return RayCastHit(RayCastHit::Type::terrain, mScene.mTerrain.get(), travelDistance, sampleAt);
		}
	}

	return RayCastHit();
}

Framework::DebugDrawer::DebugDrawer(Scene& scene) :
	mScene(scene)
{
}

void Framework::DebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	mScene.mCamera->RequestDebugLineDraw(Math::ToGLM(from), Math::ToGLM(to), Math::ToGLM(color));
}

void Framework::DebugDrawer::drawContactPoint(const btVector3&, const btVector3&, btScalar, int, const btVector3&)
{
	LOGMESSAGE("Drawing contact point");
}

void Framework::DebugDrawer::reportErrorWarning(const char*)
{
}

void Framework::DebugDrawer::draw3dText(const btVector3&, const char*)
{
}