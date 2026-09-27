#include "Physics/CharacterMovement.h"

#include "Core/Log.h"
#include "World/CollisionTrace.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <cstddef>
#include <format>
#include <string>

namespace Abomination::Physics
{
    namespace
    {
        // How many times the box may hit something and go on in one move, and how many surfaces it remembers meanwhile
        // (the same numbers as Quake).
        constexpr int MaximumBumpCount = 4;
        constexpr std::size_t MaximumPlaneCount = 5;

        // A horizontal speed below this (1 mm/s) counts as standing: friction stops it completely instead of dividing by
        // an almost zero speed.
        constexpr float StandingSpeed = 0.001f;
    }

    glm::vec3 ClipVelocity(const glm::vec3& velocity, const glm::vec3& normal)
    {
        // dot(velocity, normal) is how fast the box moves along the normal: negative into the surface. Subtracting that
        // much of the normal leaves only the part along the surface.
        const float intoSurface = glm::dot(velocity, normal);
        if (intoSurface >= 0.0f)
            return velocity;

        return velocity - normal * intoSurface;
    }

    void SlideMove(std::span<const World::CollisionBrush> brushes, glm::dvec3& position, glm::vec3& velocity,
                   const glm::dvec3& halfExtents, float deltaTime)
    {
        // The velocity the move started with: if clipping ever turns the box against it, it is stuck in a corner.
        const glm::vec3 startVelocity = velocity;

        // The velocity when the box last moved freely. Every surface clips this one, so clipping against one surface
        // does not stack up with the clips before.
        glm::vec3 freeVelocity = velocity;

        // The surfaces hit since the box last moved freely, to keep the velocity along all of them at once.
        std::array<glm::vec3, MaximumPlaneCount> planes;
        std::size_t planeCount = 0;

        float timeLeft = deltaTime;
        for (int bump = 0; bump < MaximumBumpCount; ++bump)
        {
            if (velocity == glm::vec3(0.0f))
                break;

            const glm::dvec3 end = position + glm::dvec3(velocity) * static_cast<double>(timeLeft);
            const World::TraceResult trace = World::TraceBox(brushes, position, end, halfExtents);

            // Inside a brush with no way out: do not move at all (the next steps of movement may still get it out).
            if (trace.isStuck)
            {
                velocity = glm::vec3(0.0f);
                return;
            }

            // Moved some distance: the surfaces hit before do not matter any more.
            if (trace.fraction > 0.0)
            {
                position = trace.endPosition;
                freeVelocity = velocity;
                planeCount = 0;
            }

            // Got to the end without hitting anything.
            if (trace.fraction >= 1.0)
                break;

            // Hit something: the rest of the time is spent sliding along it.
            timeLeft -= timeLeft * static_cast<float>(trace.fraction);

            if (planeCount >= MaximumPlaneCount)
            {
                velocity = glm::vec3(0.0f);
                break;
            }
            planes[planeCount++] = glm::vec3(trace.hitNormal);

            // Find a velocity along all surfaces hit so far: clip against each one and check it does not go into any of
            // the others. The first that works is taken.
            bool isFound = false;
            for (std::size_t index = 0; index < planeCount && !isFound; ++index)
            {
                const glm::vec3 clipped = ClipVelocity(freeVelocity, planes[index]);

                isFound = true;
                for (std::size_t other = 0; other < planeCount; ++other)
                    if (other != index && glm::dot(clipped, planes[other]) < 0.0f)
                        isFound = false;

                if (isFound)
                    velocity = clipped;
            }

            if (!isFound)
            {
                // No single surface works: two walls meeting at an angle (a corner, a crease). The only direction along
                // both is the line where they meet, the cross product of their normals; keep the part of the velocity
                // along it. With three or more surfaces there is no such line: stop.
                if (planeCount != 2)
                {
                    velocity = glm::vec3(0.0f);
                    break;
                }

                const glm::vec3 crease = glm::normalize(glm::cross(planes[0], planes[1]));
                velocity = crease * glm::dot(crease, velocity);
            }

            // Turned against the direction the move started with: the box would bounce back and forth in a corner.
            if (glm::dot(velocity, startVelocity) <= 0.0f)
            {
                velocity = glm::vec3(0.0f);
                break;
            }
        }
    }

    float StepSlideMove(std::span<const World::CollisionBrush> brushes, glm::dvec3& position, glm::vec3& velocity,
                        const glm::dvec3& halfExtents, float stepHeight, float deltaTime)
    {
        const glm::dvec3 startPosition = position;
        const glm::vec3 startVelocity = velocity;
        const glm::dvec3 stepUp(0.0, stepHeight, 0.0);

        // 1. The plain move, as if there were no steps.
        glm::dvec3 plainPosition = startPosition;
        glm::vec3 plainVelocity = startVelocity;
        SlideMove(brushes, plainPosition, plainVelocity, halfExtents, deltaTime);

        // 2. The same move lifted by the step height: up (as far as the ceiling allows), across, and down again.
        const World::TraceResult up = World::TraceBox(brushes, startPosition, startPosition + stepUp, halfExtents);
        glm::dvec3 steppedPosition = up.endPosition;
        glm::vec3 steppedVelocity = startVelocity;
        SlideMove(brushes, steppedPosition, steppedVelocity, halfExtents, deltaTime);

        const World::TraceResult down = World::TraceBox(brushes, steppedPosition, steppedPosition - stepUp, halfExtents);
        steppedPosition = down.endPosition;

        // Coming down on something too steep to stand on is not a step: keep the plain move.
        const bool landsOnGround = down.fraction < 1.0 && down.hitNormal.y >= MinimumGroundNormalY;

        // 3. The move that got further horizontally wins. On flat ground both are the same; at a step the plain move is
        // stopped by it and the lifted one goes over it.
        const auto horizontalDistance = [&startPosition](const glm::dvec3& end)
        {
            const glm::dvec3 offset = end - startPosition;
            return offset.x * offset.x + offset.z * offset.z; // squared: only compared, so no square root is needed
        };

        if (!landsOnGround || horizontalDistance(plainPosition) >= horizontalDistance(steppedPosition))
        {
            position = plainPosition;
            velocity = plainVelocity;
            return 0.0f;
        }

        const auto steppedUpHeight = static_cast<float>(steppedPosition.y - startPosition.y);
        position = steppedPosition;

        // The vertical speed of the plain move: going up the step must not launch the character upwards.
        velocity = glm::vec3(steppedVelocity.x, plainVelocity.y, steppedVelocity.z);

        return glm::max(steppedUpHeight, 0.0f);
    }

    void ApplyFriction(glm::vec3& velocity, const MovementSettings& settings, float deltaTime)
    {
        const float speed = glm::length(glm::vec2(velocity.x, velocity.z));
        if (speed < StandingSpeed)
        {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            return;
        }

        // Below stopSpeed the character slows down as if it moved at stopSpeed: faster at the end, so it stops soon
        // instead of losing a smaller and smaller part of an ever smaller speed.
        const float control = glm::max(speed, settings.stopSpeed);
        const float newSpeed = glm::max(speed - control * settings.friction * deltaTime, 0.0f);

        // Scale the horizontal velocity to the new speed, keeping its direction.
        const float scale = newSpeed / speed;
        velocity.x *= scale;
        velocity.z *= scale;
    }

    void Accelerate(glm::vec3& velocity, const glm::vec3& wishDirection, float wishSpeed, float acceleration,
                    float deltaTime)
    {
        // How fast the character already moves in the wished direction, and how much is missing to the wished speed.
        const float currentSpeed = glm::dot(velocity, wishDirection);
        const float missingSpeed = wishSpeed - currentSpeed;
        if (missingSpeed <= 0.0f)
            return;

        // At most acceleration * wishSpeed per second, and never more than what is missing.
        const float addedSpeed = glm::min(acceleration * wishSpeed * deltaTime, missingSpeed);
        velocity += wishDirection * addedSpeed;
    }

    bool IsOnGround(std::span<const World::CollisionBrush> brushes, const glm::dvec3& position,
                    const glm::dvec3& halfExtents)
    {
        const glm::dvec3 below = position - glm::dvec3(0.0, GroundCheckDistance, 0.0);
        const World::TraceResult trace = World::TraceBox(brushes, position, below, halfExtents);

        return trace.fraction < 1.0 && trace.hitNormal.y >= MinimumGroundNormalY;
    }

    void AirAccelerate(glm::vec3& velocity, const glm::vec3& wishDirection, float wishSpeed, float maxWishSpeed,
                       float acceleration, float deltaTime)
    {
        // Like Accelerate, with one difference: the missing speed is counted up to maxWishSpeed, while the rate still
        // uses the full wishSpeed. Turning the wished direction keeps the missing speed high (the velocity along the new
        // direction is small), which is what lets a jump bend and speed up while strafing.
        const float currentSpeed = glm::dot(velocity, wishDirection);
        const float missingSpeed = glm::min(wishSpeed, maxWishSpeed) - currentSpeed;
        if (missingSpeed <= 0.0f)
            return;

        const float addedSpeed = glm::min(acceleration * wishSpeed * deltaTime, missingSpeed);
        velocity += wishDirection * addedSpeed;
    }

    bool IsInSolid(std::span<const World::CollisionBrush> brushes, const glm::dvec3& position,
                   const glm::dvec3& halfExtents)
    {
        // A trace that does not move at all tells whether the box is inside a brush where it stands.
        return World::TraceBox(brushes, position, position, halfExtents).startsInSolid;
    }

    bool PushOutOfSolid(std::span<const World::CollisionBrush> brushes, glm::dvec3& position,
                        const glm::dvec3& halfExtents)
    {
        // Distances from an eighth of a unit (4 mm) to a whole unit (3 cm), the smallest first, so the box moves as little
        // as possible.
        constexpr std::array<double, 4> Distances = {Core::MapUnitsToMeters(1.0 / 8.0), Core::MapUnitsToMeters(1.0 / 4.0),
                                                     Core::MapUnitsToMeters(1.0 / 2.0), Core::MapUnitsToMeters(1.0)};

        for (const double distance : Distances)
            for (int x = -1; x <= 1; ++x)
                for (int y = -1; y <= 1; ++y)
                    for (int z = -1; z <= 1; ++z)
                    {
                        if (x == 0 && y == 0 && z == 0)
                            continue;

                        const glm::dvec3 candidate = position + glm::dvec3(x, y, z) * distance;
                        if (!IsInSolid(brushes, candidate, halfExtents))
                        {
                            position = candidate;
                            return true;
                        }
                    }

        return false;
    }

    void UpdateCharacter(CharacterBody& body, Core::Transform& transform, std::span<const World::CollisionBrush> brushes,
                         const PhysicsSettings& physicsSettings, const MovementSettings& movementSettings,
                         const MoveCommand& command, float deltaTime)
    {
        // Added after the player got stuck at a wall of the test map once (2026-09-27): the box was not visibly in the
        // wall, but could not move in any direction, and walking into the same place again did not repeat it. A box that
        // starts a tick inside a brush makes every trace fail, so it cannot move until it is out. Two things happen then:
        //   - a warning with everything known about the case goes to the log (once per case, not every tick), so the
        //     next time it happens there is data to reproduce it and turn it into a test;
        //   - the box is pushed out by a few millimeters if possible, so the player is not stuck forever meanwhile.
        glm::dvec3 startPosition(transform.position);
        const bool isInSolid = IsInSolid(brushes, startPosition, body.halfExtents);
        if (isInSolid && !body.isInSolid)
        {
            // Which brushes the box overlaps, by trying them one at a time.
            std::string brushIndices;
            for (std::size_t index = 0; index < brushes.size(); ++index)
                if (IsInSolid(brushes.subspan(index, 1), startPosition, body.halfExtents))
                    brushIndices += std::format("{} ", index);

            Core::Log::Write(Core::LogCategory::Physics, Core::LogLevel::Warning,
                             "Character inside brush(es) {}at ({:.4f}, {:.4f}, {:.4f}), velocity ({:.3f}, {:.3f}, {:.3f}), "
                             "on ground: {}, stepped up last tick: {:.4f}",
                             brushIndices, startPosition.x, startPosition.y, startPosition.z, body.velocity.x,
                             body.velocity.y, body.velocity.z, body.isOnGround, body.steppedUpHeight);
        }
        body.isInSolid = isInSolid;

        if (isInSolid && PushOutOfSolid(brushes, startPosition, body.halfExtents))
        {
            Core::Log::Write(Core::LogCategory::Physics, Core::LogLevel::Info,
                             "Character pushed out of the brush to ({:.4f}, {:.4f}, {:.4f})", startPosition.x,
                             startPosition.y, startPosition.z);
            transform.position = glm::vec3(startPosition);
        }

        // The wished direction may be shorter than 1 (a gamepad stick pushed half way): it scales the wished speed.
        const float wishLength = glm::length(command.wishDirection);
        const glm::vec3 wishDirection = wishLength > 0.0f ? command.wishDirection / wishLength : glm::vec3(0.0f);
        const float wishSpeed = movementSettings.maxSpeed * glm::min(wishLength, 1.0f);

        // A jump leaves the ground in this very tick: no friction for it, so a jump started at full speed keeps it. Pressing
        // jump again right on landing loses almost nothing to friction (the base of "bunny hopping" in Quake).
        if (body.isOnGround && command.wantsToJump)
        {
            body.velocity.y = movementSettings.jumpSpeed;
            body.isOnGround = false;
        }

        if (body.isOnGround)
        {
            // Gravity would keep pushing the box into the floor every tick, and on a slope the push would turn into
            // sliding down it: on the ground it does nothing, and the vertical speed is dropped.
            body.velocity.y = glm::max(body.velocity.y, 0.0f);
            ApplyFriction(body.velocity, movementSettings, deltaTime);
            Accelerate(body.velocity, wishDirection, wishSpeed, movementSettings.groundAcceleration, deltaTime);
        }
        else
        {
            AirAccelerate(body.velocity, wishDirection, wishSpeed, movementSettings.maxAirWishSpeed,
                          movementSettings.airAcceleration, deltaTime);
            body.velocity.y -= physicsSettings.gravity * deltaTime;
        }

        glm::dvec3 position(transform.position);
        // Steps are walked up on the ground, and also in the air while not moving up, like in Quake 2 and 3: a player who
        // jumps onto stairs lands on them and runs on, instead of hitting the edge of a step like a wall and losing all
        // speed. While moving up (the first half of a jump) stepping is off, or a jump next to a wall would climb it one
        // step at a time. (The other way to smooth stairs is in the map: an invisible slope over them, "player clip".)
        body.steppedUpHeight = 0.0f;
        const bool canStepUp = body.isOnGround || body.velocity.y <= 0.0f;
        if (canStepUp)
            body.steppedUpHeight = StepSlideMove(brushes, position, body.velocity, body.halfExtents,
                                                 movementSettings.stepHeight, deltaTime);
        else
            SlideMove(brushes, position, body.velocity, body.halfExtents, deltaTime);
        transform.position = glm::vec3(position);

        body.isOnGround = body.velocity.y <= MaximumGroundUpwardSpeed && IsOnGround(brushes, position, body.halfExtents);
    }
}
