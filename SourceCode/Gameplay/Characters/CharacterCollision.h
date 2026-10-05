#pragma once

#include "World/CollisionBrush.h"

#include <entt/entt.hpp>

#include <span>
#include <vector>

namespace Abomination::Gameplay
{
    // The boxes of the characters a character bumps into: every entity with a Physics::CharacterBody except mover, so
    // characters stop at each other like at walls instead of walking through. Bodies of dead monsters (Corpse) are left
    // out: they are walked through. Together with the brushes of the level they make the World::CollisionWorld it moves
    // through; the level itself is not copied, only these few boxes are made every tick.
    //
    // The boxes are made in memory, which is reused from call to call (GameplayState::characterBoxMemory): it only
    // grows, so after the first ticks gathering allocates nothing. The result is the part of memory that was filled,
    // valid until the next call with the same memory.
    //
    // A large level with many enemies will want a spatial search (a grid or a tree) that hands out only the brushes and
    // boxes near the character (see ROADMAP 0.2 notes).
    [[nodiscard]] std::span<const World::CollisionBrush> GatherCharacterBoxes(const entt::registry& registry,
                                                                              entt::entity mover,
                                                                              std::vector<World::CollisionBrush>& memory);
}
