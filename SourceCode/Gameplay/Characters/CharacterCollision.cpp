#include "Gameplay/Characters/CharacterCollision.h"

#include "Core/Profiling/ProfileZone.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/Enemies/Corpses.h"
#include "Physics/CharacterBody.h"

namespace Abomination::Gameplay
{
    std::vector<World::CollisionBrush> GatherCharacterBoxes(const entt::registry& registry, entt::entity mover)
    {
        PROFILE_ZONE();

        std::vector<World::CollisionBrush> boxes;
        const auto characters = registry.view<const Physics::CharacterBody, const Core::Transform>(entt::exclude<Corpse>);
        for (const auto [entity, body, transform] : characters.each())
            if (entity != mover)
                boxes.push_back(World::CreateBoxCollisionBrush(glm::dvec3(transform.position), body.halfExtents));

        return boxes;
    }
}
