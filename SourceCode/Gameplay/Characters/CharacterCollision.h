#pragma once

#include "World/CollisionBrush.h"

#include <entt/entt.hpp>

#include <span>
#include <vector>

namespace Abomination::Gameplay
{
    // The brushes a character moves through: those of the level and the box of every other character (every entity with
    // a Physics::CharacterBody except mover), so characters stop at each other like at walls instead of walking through.
    //
    // The level brushes are copied for every character every tick. With a level of a few dozen brushes and a few
    // characters it costs nothing; a large level with many enemies will want a spatial search (a grid or a tree) that
    // hands out only the brushes near the character, together with the one for the level itself (see ROADMAP 0.2 notes).
    [[nodiscard]] std::vector<World::CollisionBrush> GatherCollisionBrushes(const entt::registry& registry,
                                                                            std::span<const World::CollisionBrush> level,
                                                                            entt::entity mover);
}
