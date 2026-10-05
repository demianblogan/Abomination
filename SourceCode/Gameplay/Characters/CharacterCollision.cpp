#include "Gameplay/Characters/CharacterCollision.h"

#include "Core/Profiling/ProfileZone.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/Enemies/Corpses.h"
#include "Physics/CharacterBody.h"

#include <cstddef>

namespace Abomination::Gameplay
{
    std::span<const World::CollisionBrush> GatherCharacterBoxes(const entt::registry& registry, entt::entity mover,
                                                                std::vector<World::CollisionBrush>& memory)
    {
        PROFILE_ZONE();

        // The boxes are shaped over the ones of the last call; a new one is added only when there are more characters
        // than ever before. Memory never shrinks: a removed box would free its planes, and the next call would allocate
        // them again.
        std::size_t count = 0;
        const auto characters = registry.view<const Physics::CharacterBody, const Core::Transform>(entt::exclude<Corpse>);
        for (const auto [entity, body, transform] : characters.each())
        {
            if (entity == mover)
                continue;

            if (count == memory.size())
                memory.emplace_back();
            World::ShapeBoxCollisionBrush(memory[count], glm::dvec3(transform.position), body.halfExtents);
            ++count;
        }

        return std::span<const World::CollisionBrush>(memory).first(count);
    }
}
