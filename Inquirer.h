#pragma once
#include <vector>
#include <algorithm>
#include <set>

namespace Framework
{
	// Needed for queries
	class Inquirer :
		public btCollisionWorld::ContactResultCallback
	{
	public:
		btScalar addSingleResult(btManifoldPoint&, const btCollisionObjectWrapper*, int, int, const btCollisionObjectWrapper* colObj1Wrap, int, int) override
		{
			const btCollisionObject* obj = colObj1Wrap->getCollisionObject();

			for (size_t i = mCollidedWith.size(); i-- > 0;)
			{
				if (mCollidedWith[i] == obj)
				{
					return {};
				}
			}
			mCollidedWith.push_back(obj);

			// Bullet physics doesn't use the return value.
			return {};
		}

		std::vector<const btCollisionObject*> mCollidedWith{};
		btCollisionObject mCollisionObject{};
	}; 
}