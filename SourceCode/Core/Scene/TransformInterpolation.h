#pragma once

#include "Core/Scene/Transform.h"

#include <entt/entt.hpp>

// Smooth drawing of entities that move in fixed ticks (see Core::FixedTimestep and ARCHITECTURE.md, "Main loop").
//
// The simulation changes Transform only in ticks, 60 times per second, while frames are drawn at any rate. Drawing the
// state after the last tick would make movement jerk at frame rates that do not match the tick rate. So an entity that
// moves also remembers where it was before the last tick (PreviousTransform), and every frame is drawn between the two:
//
//   before every tick:  StorePreviousTransforms()       previous = current
//   the tick:           systems change Transform         current = new state
//   every frame:        InterpolateTransform(previous, current, alpha)   alpha = FixedTimestep::GetInterpolationFactor()
//
// Only moving entities need it: a crate that never moves is drawn from its Transform directly.
namespace Abomination::Core
{
    // Component: the Transform of the entity before the last simulation tick.
    struct PreviousTransform
    {
        Transform value;
    };

    // Makes a moving entity be drawn smoothly: adds a PreviousTransform equal to its current Transform, so the first
    // frames do not blend from some other, stale state. The entity must already have a Transform.
    void EnableInterpolation(entt::registry& registry, entt::entity entity);

    // The interpolation system, part 1: copies Transform into PreviousTransform for every entity that has both.
    // Must run at the start of every tick, before any system moves anything.
    void StorePreviousTransforms(entt::registry& registry);

    // The interpolation system, part 2: the transform at fraction alpha (0 to 1) of the way from previous to current.
    // Position and scale are blended in a straight line (glm::mix), the rotation along the shortest arc (glm::slerp).
    [[nodiscard]] Transform InterpolateTransform(const Transform& previous, const Transform& current, float alpha);
}
