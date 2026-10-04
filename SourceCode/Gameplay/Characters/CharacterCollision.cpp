#include "Gameplay/Characters/CharacterCollision.h"

#include "Core/Scene/Transform.h"
#include "Gameplay/Enemies/Monsters.h"
#include "Physics/CharacterBody.h"

namespace Abomination::Gameplay
{
    std::vector<World::CollisionBrush> GatherCollisionBrushes(const entt::registry& registry,
                                                              std::span<const World::CollisionBrush> level,
                                                              entt::entity mover)
    {
        std::vector<World::CollisionBrush> brushes(level.begin(), level.end());
        // Bodies of dead monsters are walked through (see Corpse).
        const auto characters = registry.view<const Physics::CharacterBody, const Core::Transform>(entt::exclude<Corpse>);
        for (const auto [entity, body, transform] : characters.each())
            if (entity != mover)
                brushes.push_back(World::CreateBoxCollisionBrush(glm::dvec3(transform.position), body.halfExtents));

        return brushes;
    }
}
