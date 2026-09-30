#include "Gameplay/CharacterCollision.h"

#include "Core/Scene/Transform.h"
#include "Physics/CharacterBody.h"

namespace Abomination::Gameplay
{
    std::vector<World::CollisionBrush> GatherCollisionBrushes(const entt::registry& registry,
                                                              std::span<const World::CollisionBrush> level,
                                                              entt::entity mover)
    {
        std::vector<World::CollisionBrush> brushes(level.begin(), level.end());
        for (const auto [entity, body, transform] : registry.view<const Physics::CharacterBody, const Core::Transform>().each())
            if (entity != mover)
                brushes.push_back(World::CreateBoxCollisionBrush(glm::dvec3(transform.position), body.halfExtents));

        return brushes;
    }
}
