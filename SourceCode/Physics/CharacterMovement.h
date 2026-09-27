#pragma once

#include "Core/Transform.h"
#include "Physics/CharacterBody.h"
#include "World/CollisionBrush.h"

#include <glm/vec3.hpp>

#include <span>

namespace Abomination::Physics
{
    // Settings of the physical world, the same for everything that falls. Changed in the debug overlay; later a map may
    // set its own gravity (a property of worldspawn).
    struct PhysicsSettings
    {
        // Meters per second squared, downwards. Quake uses 800 units/s², that is 25 m/s²: much more than the real 9.81,
        // which makes jumps short and snappy instead of floaty.
        float gravity = 25.0f;
    };

    // A surface is ground (the character can stand and walk on it) when its normal points up at least this much: the y
    // of the normal is the cosine of the slope angle, and 0.7 is about 45 degrees. Steeper slopes are walls: the
    // character slides down them. The same value as Quake.
    inline constexpr double MinimumGroundNormalY = 0.7;

    // How far below the box the ground is looked for: a quarter of a unit, as in Quake 2. It must be more than the gap
    // a trace leaves before a surface (World::SurfaceEpsilon), or a character standing on the floor would not find it.
    inline constexpr double GroundCheckDistance = 0.25 / 32.0;

    // A character moving up faster than this is not on the ground even if the ground is right below it (the first
    // moments of a jump). 180 units/s, as in Quake.
    inline constexpr float MaximumGroundUpwardSpeed = 180.0f / 32.0f;

    // The velocity with the part that goes into the surface removed: what is left moves along the surface. normal
    // points out of the surface. Moving away from the surface (or along it) does not change the velocity.
    [[nodiscard]] glm::vec3 ClipVelocity(const glm::vec3& velocity, const glm::vec3& normal);

    // Moves the box along its velocity for deltaTime and slides it along what it hits, instead of stopping: at every hit
    // the velocity is clipped to the surface (ClipVelocity) and the box continues with the rest of the time, up to 4 hits
    // per call. Hitting two surfaces at once (a corner of two walls) leaves only the movement along the line where they
    // meet. The same algorithm as PM_FlyMove of Quake. position and velocity are updated.
    void SlideMove(std::span<const World::CollisionBrush> brushes, glm::dvec3& position, glm::vec3& velocity,
                   const glm::dvec3& halfExtents, float deltaTime);

    // Whether a box at position stands on the ground: something walkable (see MinimumGroundNormalY) is directly below it,
    // within GroundCheckDistance.
    [[nodiscard]] bool IsOnGround(std::span<const World::CollisionBrush> brushes, const glm::dvec3& position,
                                  const glm::dvec3& halfExtents);

    // One tick of a character: gravity pulls it down while it is in the air, it slides through the level along its
    // velocity (SlideMove), and then it checks whether it stands on the ground. transform.position is the center of its
    // box.
    void UpdateCharacter(CharacterBody& body, Core::Transform& transform, std::span<const World::CollisionBrush> brushes,
                         const PhysicsSettings& settings, float deltaTime);
}
