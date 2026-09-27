#pragma once

#include "Core/Units.h"

#include <glm/vec3.hpp>

// The player as the map knows them: the info_player_start entity. Its box is defined in the entity definitions for
// TrenchBroom (Tools/TrenchBroom/Abomination/Abomination.fgd) the same way as in Quake:
//
//   size(-16 -16 -24, 16 16 32)      32 x 32 units across, from 24 units below the origin of the entity to 32 above it
//
//        +32  ---+---          the box is 56 units tall, about 1.75 m
//                |
//        +4   ---+--- center   (-24 + 32) / 2 = +4: the center is 4 units above the origin, not at it
//         0   ---+--- origin   the point the map stores for the entity
//                |
//        -24  ---+--- bottom   a player start placed on the floor in TrenchBroom has its bottom exactly on the floor
namespace Abomination::World
{
    // Half the size of the player box in meters: 16 x 28 x 16 units, that is 0.5 x 0.875 x 0.5 m.
    inline constexpr glm::dvec3 PlayerHalfExtents{Core::MapUnitsToMeters(16.0), Core::MapUnitsToMeters(28.0),
                                                  Core::MapUnitsToMeters(16.0)};

    // How far the center of the player box is above the origin of info_player_start, in map units (see above).
    inline constexpr double PlayerBoxCenterAboveOrigin = 4.0;

    // Where the player appears and where they look, in game coordinates (from the info_player_start entity).
    struct PlayerStart
    {
        // The center of the player box: the position of the player entity (see Physics::CharacterBody).
        glm::vec3 boxCenter{0.0f};

        // Radians, like every yaw of the game (see Gameplay/MouseLook.h).
        float yaw = 0.0f;
    };
}
