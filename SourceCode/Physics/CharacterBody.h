#pragma once

#include <glm/vec3.hpp>

namespace Abomination::Physics
{
    // Component: a character that walks through the level as a box: the player now, enemies later. The position of the
    // box is the position of the entity's Core::Transform (the center of the box); the box never turns, like in Quake,
    // so it slides along walls the same way whichever way the character looks.
    struct CharacterBody
    {
        // Half the size of the box along each axis, in meters. Set by whoever creates the character: the player gets
        // World::PlayerHalfExtents, the box of info_player_start in the map editor.
        glm::dvec3 halfExtents{0.0};

        // Meters per second. Changed by the movement code every tick (walking, gravity, jumping, hitting walls).
        glm::vec3 velocity{0.0f};

        // The box stands on a surface flat enough to walk on. Walking, friction and jumping depend on it.
        bool isOnGround = false;

        // How high the box walked up a step in the last tick (0 if it did not). The view of the player uses it to glide up
        // stairs instead of jumping (see Gameplay::StepSmoothing).
        float steppedUpHeight = 0.0f;

        // The box began the last tick inside a brush (see Physics::UpdateCharacter). Shown in the debug overlay; also keeps
        // the warning about it to one message per case.
        bool isInSolid = false;
    };
}
