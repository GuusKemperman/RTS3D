#pragma once

class btGhostPairCallback;
class btPairCachingGhostObject;

namespace Framework
{
	class Inquirer;
	class Scene;
	class Camera;
	class Transform;

	class DebugDrawer :
		public btIDebugDraw
	{
	public:
		DebugDrawer(Scene& scene);

		void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
		void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override;
		void reportErrorWarning(const char* warningString) override;
		void draw3dText(const btVector3& location, const char* textString) override;
		inline void setDebugMode(int mode) override { mMode = mode; };
		inline int getDebugMode() const override { return mMode; };

	private:
		Scene& mScene;
		int mMode{};
	};

	class Physics
	{
	public:
		Physics(Scene& scene);
		~Physics();
		
		void Tick();

		inline void SetDebugMode(int mode) { mDebugDrawer.setDebugMode(mode); }

		void DebugDraw();

		enum Group
		{
			noneGroup					= 0b0,
			cameraGroup					= 1,
			terrainGroup				= 1 << 1, 
			unitGroup					= 1 << 2,
			projectileGroup				= 1 << 3,
			staticObstacleGroup			= 1 << 4,
			visibileButNoCollisionGroup	= 1 << 5,
			agentGroup = unitGroup
		};

		enum Mask
		{
			noneMask					= 0b0,
			cameraMask					= Group::unitGroup | Group::staticObstacleGroup | Group::projectileGroup | Group::visibileButNoCollisionGroup,
			terrainMask					= Group::unitGroup | Group::projectileGroup,
			unitMask					= Group::cameraGroup | Group::terrainGroup | Group::unitGroup | Group::staticObstacleGroup | Group::projectileGroup,
			projectileMask				= Group::cameraGroup | Group::terrainGroup | Group::unitGroup | Group::staticObstacleGroup | Group::projectileGroup,
			staticObstacleMask			= Group::cameraGroup | Group::unitGroup | Group::projectileGroup,
			visibleButNoCollisionMask	= Group::cameraGroup,
			agentMask = Mask::unitMask
		};

		void AddCollisionObjectToWorld(btCollisionObject* object, Group group, Mask mask);
		void RemoveCollisionObjectFromWorld(std::unique_ptr<btCollisionObject> object);

		//-------------------------------------------------------------------------------------------------------------------------------------//
		// https://www.executionunit.com/blog/2015/03/27/bullet-physics-query-objects-with-a-volume/ My source for the queries (thanks brian!)-//
		//-------------------------------------------------------------------------------------------------------------------------------------//
		void Query(Inquirer& inquirer, const Transform& transform) const;

		//std::vector<const btCollisionObject*> GetNarrowPhaseCollisions(btPairCachingGhostObject& forObject) const;

		struct RayCastHit
		{
			enum class Type { null, terrain, entity };
			RayCastHit() = default;
			RayCastHit(Type type, void* const hit, const float distance, const glm::vec3 position) :
				mType(type),
				mHit(hit),
				mDistance(distance),
				mPosition(position)
			{
				
			}
			const Type mType{};
			void* const mHit{};
			const float mDistance = INFINITY;
			const glm::vec3 mPosition{};
		};
		RayCastHit RayCast(const glm::vec3 start, glm::vec3 direction, float maxDistance) const;

	private:
		Scene& mScene;

		DebugDrawer mDebugDrawer;

		btBroadphaseInterface* mBroadPhase{};
		btDefaultCollisionConfiguration* mCollisionConfiguration{};
		btCollisionDispatcher* mDispatcher{};
		btSequentialImpulseConstraintSolver* mConstraintSolver{};
		btDiscreteDynamicsWorld* mWorld{};	
	};
}