#include "Gameplay/Effects/Tumbling.h"

#include "World/CollisionTrace.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Abomination::Gameplay
{
    TumbleStep Tumble(Tumbler& tumbler, Core::Transform& transform, std::span<const World::CollisionBrush> brushes,
                      float gravity, double halfSize, const BounceSettings& settings, float deltaTime)
    {
        // It falls, moves as far as the level lets it in this step, and spins.
        tumbler.velocity.y -= gravity * deltaTime;
        const glm::vec3 start = transform.position;
        const World::TraceResult trace = World::TraceBox(brushes, glm::dvec3(start),
                                                         glm::dvec3(start + tumbler.velocity * deltaTime),
                                                         glm::dvec3(halfSize));
        transform.position = glm::vec3(trace.endPosition);
        const glm::quat spin = glm::angleAxis(tumbler.spinSpeed * deltaTime, tumbler.spinAxis);
        transform.rotation = glm::normalize(spin * transform.rotation);

        // Stuck in a brush (thrown into a moving door, later): it stays where it is.
        TumbleStep step;
        if (trace.isStuck)
        {
            step.hasStopped = true;
            return step;
        }
        if (trace.fraction >= 1.0)
            return step;

        // A bounce. The velocity is split into its part into the surface (along the normal) and its part along the
        // surface: the first is turned back and weakened (bounce), the second is slowed by rubbing (slide).
        const glm::vec3 normal(trace.hitNormal);
        step.hasHit = true;
        step.normal = normal;
        step.speedIntoSurface = -glm::dot(tumbler.velocity, normal);
        const glm::vec3 alongNormal = normal * glm::dot(tumbler.velocity, normal);
        const glm::vec3 alongSurface = tumbler.velocity - alongNormal;
        tumbler.velocity = alongSurface * settings.slide - alongNormal * settings.bounce;
        tumbler.spinSpeed *= settings.spinKept;

        // Slow enough on a floor: it lies down.
        if (normal.y >= FloorNormalY && glm::length(tumbler.velocity) < settings.restSpeed)
        {
            tumbler.velocity = glm::vec3(0.0f);
            step.hasStopped = true;
        }
        return step;
    }
}
