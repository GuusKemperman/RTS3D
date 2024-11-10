#pragma once
#include "Entity.h"

namespace RTS
{
    class Tree :
        public Framework::Entity
    {
        ENTITYMAKER(Tree)
    public:
        Tree(Framework::Scene& scene, const glm::vec2 position = { 0.0f, 0.0f });
        ~Tree();

        static constexpr float sMinDistBetweenTrees = 2.0f;
        static constexpr uint sNumOfTreeModels = 10u;

    private:
        static constexpr float sMinScale = .75f;
        static constexpr float sMaxScale = 1.0f;

        static constexpr float sMaxRotationXZ = TWOPI * (7.0f / 360.0f);
    };
}