#pragma once

#include "Core/Units.h"
#include "World/CollisionBrush.h"

#include <glm/vec3.hpp>

#include <span>

namespace Abomination::World
{
    // The distance a traced box stops before a surface: 1/32 of a map unit (about 1 mm), as in Quake. Without it the
    // stopped box could end up a hair inside the surface after rounding, and the next trace would start stuck in it.
    inline constexpr double SurfaceEpsilon = Core::MapUnitsToMeters(1.0 / 32.0);

    // What happened to a box moved along a straight line (see TraceBox).
    struct TraceResult
    {
        // The part of the way the box moved before it hit something: 1 = it got to the end, 0.5 = it stopped half way,
        // 0 = it could not move at all.
        double fraction = 1.0;

        // Where the center of the box stopped: start + (end - start) * fraction.
        glm::dvec3 endPosition{0.0};

        // The normal of the surface the box hit (pointing out of it, towards the box); zero if it hit nothing.
        glm::dvec3 hitNormal{0.0};

        // The box started inside a brush. It may still be able to move out of it.
        bool startsInSolid = false;

        // The whole way lies inside a brush: the box is stuck (fraction is 0).
        bool isStuck = false;
    };

    // Moves a box from start to end (both are positions of its center) through the brushes and tells where it stops.
    // halfExtents is half the size of the box along each axis (for the player 0.5, 0.875 and 0.5 m). Game meters.
    //
    // The box is turned into a point: every plane of a brush is moved out by as much as the box reaches along its
    // normal, and the center of the box is traced against the enlarged brush. For every plane the line from start to end
    // crosses, the crossing is an entry (from the front of the plane to its back) or an exit. The line is inside the
    // brush between the latest entry and the earliest exit, so the box hits the brush where it enters it, if the latest
    // entry comes before the earliest exit. Of all brushes, the earliest hit counts.
    [[nodiscard]] TraceResult TraceBox(std::span<const CollisionBrush> brushes, const glm::dvec3& start,
                                       const glm::dvec3& end, const glm::dvec3& halfExtents);
}
