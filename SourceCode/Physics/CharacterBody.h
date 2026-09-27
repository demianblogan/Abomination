#pragma once

#include <glm/vec3.hpp>

namespace Abomination::Physics
{
    // Component: a character that walks through the level as a box: the player now, enemies later. The position of the
    // box is the position of the entity's Core::Transform (the center of the box); the box never turns, like in Quake,
    // so it slides along walls the same way whichever way the character looks.
    struct CharacterBody
    {
        // Half the size of the box along each axis, in meters.
        glm::dvec3 halfExtents{0.5, 0.875, 0.5};

        // Meters per second. Changed by the movement code every tick (walking, gravity, jumping, hitting walls).
        glm::vec3 velocity{0.0f};

        // The box stands on a surface flat enough to walk on. Walking, friction and jumping depend on it.
        bool isOnGround = false;
    };
}
