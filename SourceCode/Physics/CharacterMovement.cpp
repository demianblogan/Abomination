#include "Physics/CharacterMovement.h"

#include "World/CollisionTrace.h"

#include <glm/geometric.hpp>

#include <array>
#include <cstddef>

namespace Abomination::Physics
{
    namespace
    {
        // How many times the box may hit something and go on in one move, and how many surfaces it remembers meanwhile
        // (the same numbers as Quake).
        constexpr int MaximumBumpCount = 4;
        constexpr std::size_t MaximumPlaneCount = 5;
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

    bool IsOnGround(std::span<const World::CollisionBrush> brushes, const glm::dvec3& position,
                    const glm::dvec3& halfExtents)
    {
        const glm::dvec3 below = position - glm::dvec3(0.0, GroundCheckDistance, 0.0);
        const World::TraceResult trace = World::TraceBox(brushes, position, below, halfExtents);

        return trace.fraction < 1.0 && trace.hitNormal.y >= MinimumGroundNormalY;
    }

    void UpdateCharacter(CharacterBody& body, Core::Transform& transform, std::span<const World::CollisionBrush> brushes,
                         const PhysicsSettings& settings, float deltaTime)
    {
        // Gravity only in the air. On the ground it would keep pushing the box into the floor every tick, and on a slope
        // the push would turn into sliding down it.
        if (!body.isOnGround)
            body.velocity.y -= settings.gravity * deltaTime;
        else if (body.velocity.y < 0.0f)
            body.velocity.y = 0.0f;

        glm::dvec3 position(transform.position);
        SlideMove(brushes, position, body.velocity, body.halfExtents, deltaTime);
        transform.position = glm::vec3(position);

        body.isOnGround = body.velocity.y <= MaximumGroundUpwardSpeed && IsOnGround(brushes, position, body.halfExtents);
    }
}
