#pragma once

#include "Core/Scene/Transform.h"
#include "World/CollisionBrush.h"

#include <glm/vec3.hpp>

#include <span>

// The flight of small loose things thrown about the level: the spent shells of the shotgun and the gibs of a body. They
// fall, tumble around an axis, and bounce off the level losing speed, until they lie still on a floor. Only for the
// eyes: nothing in the game depends on them.
namespace Abomination::Gameplay
{
    // A thing in flight: how it moves and spins, and how it bounces.
    struct Tumbler
    {
        glm::vec3 velocity{0.0f};

        // The axis it spins around and how fast (radians per second).
        glm::vec3 spinAxis{1.0f, 0.0f, 0.0f};
        float spinSpeed = 0.0f;
    };

    // How a thing bounces: the part of its speed into a surface it keeps (turned back), the part along the surface it
    // keeps (rubbing), the part of its spin it keeps, and below what speed on a floor it lies down.
    struct BounceSettings
    {
        float bounce = 0.35f;
        float slide = 0.6f;
        float spinKept = 0.5f;
        float restSpeed = 0.5f;
    };

    // What happened in one step of a flight.
    struct TumbleStep
    {
        // It hit something this step, how hard (meters per second into the surface), and the normal of the surface.
        bool hasHit = false;
        float speedIntoSurface = 0.0f;
        glm::vec3 normal{0.0f};

        // It came to rest: slow enough on a floor, or stuck in a brush.
        bool hasStopped = false;
    };

    // A surface is a floor a thing can lie on when its normal points up at least this much (about 45 degrees).
    inline constexpr float FloorNormalY = 0.7f;

    // One step of deltaTime seconds: gravity pulls the thing, it moves as far as the brushes let it (a box halfSize to
    // each side) and spins; on a hit the part of its velocity into the surface is turned back and weakened (bounce), the
    // part along it slowed by rubbing (slide).
    TumbleStep Tumble(Tumbler& tumbler, Core::Transform& transform, std::span<const World::CollisionBrush> brushes,
                      float gravity, double halfSize, const BounceSettings& settings, float deltaTime);
}
