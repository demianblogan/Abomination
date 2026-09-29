#pragma once

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
    // Component: the entity keeps turning around an axis. Nothing spins at the moment; pickups will turn in place with
    // it, as in Quake (0.6).
    struct Spin
    {
        // The axis in world coordinates; does not have to be normalized.
        glm::vec3 axis{0.0f, 1.0f, 0.0f};

        // Radians per second; negative turns the other way.
        float speed = 1.0f;
    };

    // The spin system: turns every entity with a Spin and a Core::Transform by speed * deltaTime.
    // Runs in FixedUpdate, deltaTime is the tick duration.
    void UpdateSpinningEntities(entt::registry& registry, float deltaTime);
}
