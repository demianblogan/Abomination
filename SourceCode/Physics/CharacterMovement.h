#pragma once

#include "Core/Math/Units.h"
#include "Core/Scene/Transform.h"
#include "Physics/CharacterBody.h"
#include "World/CollisionTrace.h"

#include <glm/vec3.hpp>

#include <optional>

namespace Abomination::Physics
{
    // Settings of the physical world, the same for everything that falls. Changed in the debug overlay; later a map may
    // set its own gravity (a property of worldspawn).
    struct PhysicsSettings
    {
        // Meters per second squared, downwards. Quake uses 800 units/s², that is 25 m/s²: much more than the real 9.81,
        // which makes jumps short and snappy instead of floaty.
        float gravity = Core::MapUnitsToMeters(800.0f);
    };

    // How a character walks. The formulas are those of Quake, and so are most values; the running speed is that of
    // modern shooters instead, because Quake is very fast. Changed in the debug overlay; later read from the
    // configuration of every kind of character (enemies walk slower).
    struct MovementSettings
    {
        // The fastest a character can run on its own, in meters per second. Modern shooters run at about 6-7 m/s
        // (Counter-Strike 6.4, Valorant 6.75); Quake runs at 10 m/s (320 units/s), faster than a sprinter.
        float maxSpeed = 7.0f;

        // How fast the character reaches its speed on the ground: every second it may gain groundAcceleration times its
        // wished speed (10 in Quake, so full speed in a tenth of a second).
        float groundAcceleration = 10.0f;

        // How fast a character on the ground slows down without input: every second it loses friction times its speed
        // (4 in Quake), but at least friction times stopSpeed, so it stops completely instead of creeping forever.
        float friction = 4.0f;
        float stopSpeed = Core::MapUnitsToMeters(100.0f);

        // The highest step a character walks up without jumping: 18 units in Quake, about 0.56 m.
        float stepHeight = Core::MapUnitsToMeters(18.0f);

        // The upward speed a jump starts with: 270 units/s in Quake. With 25 m/s² of gravity the jump reaches
        // jumpSpeed² / (2 × gravity) = about 1.4 m.
        float jumpSpeed = Core::MapUnitsToMeters(270.0f);

        // Control in the air, the way Quake does it: the character may only add speed up to maxAirWishSpeed in the wished
        // direction (30 units/s, about 1 m/s), but at the full rate of airAcceleration. That is too little to fly anywhere
        // you like, but enough to bend a jump by turning the mouse while holding a strafe key ("air strafing").
        float airAcceleration = 10.0f;
        float maxAirWishSpeed = Core::MapUnitsToMeters(30.0f);
    };

    // What a character wants to do in one tick: from the keys (the player) or from the mind of a monster (DogMind). The movement
    // code decides what really happens.
    struct MoveCommand
    {
        // Where the character wants to go: horizontal, length 1 to go at full speed, 0 to stand.
        glm::vec3 wishDirection{0.0f};

        // Jump now, if standing on the ground. In the air the wish is ignored (no double jumps).
        bool wantsToJump = false;
    };

    // A surface is ground (the character can stand and walk on it) when its normal points up at least this much: the y
    // of the normal is the cosine of the slope angle, and 0.7 is about 45 degrees. Steeper slopes are walls: the
    // character slides down them. The same value as Quake.
    inline constexpr double MinimumGroundNormalY = 0.7;

    // How far below the box the ground is looked for: a quarter of a unit, as in Quake 2. It must be more than the gap
    // a trace leaves before a surface (World::SurfaceEpsilon), or a character standing on the floor would not find it.
    inline constexpr double GroundCheckDistance = Core::MapUnitsToMeters(0.25);

    // A character moving up faster than this is not on the ground even if the ground is right below it (the first
    // moments of a jump). 180 units/s, as in Quake.
    inline constexpr float MaximumGroundUpwardSpeed = Core::MapUnitsToMeters(180.0f);

    // The velocity with the part that goes into the surface removed: what is left moves along the surface. normal
    // points out of the surface. Moving away from the surface (or along it) does not change the velocity.
    [[nodiscard]] glm::vec3 ClipVelocity(const glm::vec3& velocity, const glm::vec3& normal);

    // Moves the box along its velocity for deltaTime and slides it along what it hits, instead of stopping: at every hit
    // the velocity is clipped to the surface (ClipVelocity) and the box continues with the rest of the time, up to 4 hits
    // per call. Hitting two surfaces at once (a corner of two walls) leaves only the movement along the line where they
    // meet. The same algorithm as PM_FlyMove of Quake. position and velocity are updated.
    void SlideMove(const World::CollisionWorld& world, glm::dvec3& position, glm::vec3& velocity,
                   const glm::dvec3& halfExtents, float deltaTime);

    // Like SlideMove, but also walks up steps: the move is tried twice, as it is and lifted by stepHeight (then put down
    // again), and the one that got further horizontally wins. On flat ground both give the same, at a step the lifted one
    // gets over it. The same approach as PM_StepSlideMove of Quake 2. For characters on the ground, or in the air while
    // not moving up (landing on stairs).
    // Returns how high the box walked up (0 if the plain move won).
    float StepSlideMove(const World::CollisionWorld& world, glm::dvec3& position, glm::vec3& velocity,
                       const glm::dvec3& halfExtents, float stepHeight, float deltaTime);

    // Slows down the horizontal velocity of a character on the ground (see MovementSettings::friction).
    void ApplyFriction(glm::vec3& velocity, const MovementSettings& settings, float deltaTime);

    // Speeds the velocity up towards wishSpeed in wishDirection (length 1), by at most acceleration * wishSpeed per
    // second. Only the part of the velocity along wishDirection counts towards the wished speed: turning keeps momentum,
    // and a character already faster than wishSpeed in that direction is not slowed down. The accelerate function of
    // Quake.
    void Accelerate(glm::vec3& velocity, const glm::vec3& wishDirection, float wishSpeed, float acceleration,
                    float deltaTime);

    // Accelerate for a character in the air: the missing speed is counted up to maxWishSpeed only, but the rate is that
    // of the full wishSpeed (see MovementSettings::maxAirWishSpeed). The air-accelerate function of Quake.
    void AirAccelerate(glm::vec3& velocity, const glm::vec3& wishDirection, float wishSpeed, float maxWishSpeed,
                       float acceleration, float deltaTime);

    // Whether a box at position overlaps a brush.
    [[nodiscard]] bool IsInSolid(const World::CollisionWorld& world, const glm::dvec3& position,
                                 const glm::dvec3& halfExtents);

    // Tries to move a box that overlaps a brush out of it by a tiny distance: up to 1 unit (3 cm) along any of the 26
    // directions to the sides, edges and corners of a cube, the shortest distances first. Returns true and updates position
    // if a free place was found. A safety net for mistakes of the movement code, like PM_NudgePosition of Quake 2: without
    // it a box that got a hair into a wall would stay stuck there forever, because every trace starting inside a brush fails.
    [[nodiscard]] bool PushOutOfSolid(const World::CollisionWorld& world, glm::dvec3& position,
                                      const glm::dvec3& halfExtents);

    // Whether a box at position stands on the ground: something walkable (see MinimumGroundNormalY) is directly below it,
    // within GroundCheckDistance.
    [[nodiscard]] bool IsOnGround(const World::CollisionWorld& world, const glm::dvec3& position,
                                  const glm::dvec3& halfExtents);

    // The same, giving the normal of that ground; none if there is none.
    [[nodiscard]] std::optional<glm::vec3> FindGroundNormal(const World::CollisionWorld& world,
                                                            const glm::dvec3& position, const glm::dvec3& halfExtents);

    // One tick of a character: a jump starts if it is wished and the character stands on the ground; on the ground,
    // friction slows it down and the command speeds it up; in the air, gravity pulls it down and the command bends the
    // way a little (AirAccelerate). Then it moves through the level (stepping up stairs on the ground, sliding along
    // walls) and checks whether it stands on the ground. transform.position is the center of its box.
    void UpdateCharacter(CharacterBody& body, Core::Transform& transform, const World::CollisionWorld& world,
                         const PhysicsSettings& physicsSettings, const MovementSettings& movementSettings,
                         const MoveCommand& command, float deltaTime);
}
