#include "World/CollisionTrace.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>

#include <algorithm>
#include <cassert>

namespace Abomination::World
{
    namespace
    {
        // How far a box reaches from its center along a direction: the half sizes, each weighted by how much the
        // direction points along that axis. For the normal (1, 0, 0) it is halfExtents.x; for a slope at 45 degrees
        // between X and Y it is 0.707 * (halfExtents.x + halfExtents.y), because a corner of the box sticks out furthest.
        double CalculateReach(const glm::dvec3& normal, const glm::dvec3& halfExtents)
        {
            return glm::dot(glm::abs(normal), halfExtents);
        }

        // Everything the moving box touches stays inside the box around its whole way: a quick test to skip brushes
        // far away from it.
        //
        // The way is made SurfaceEpsilon larger on every side: a trace must also see brushes it only comes closer to than
        // SurfaceEpsilon, because it stops that far in front of them. Without it, a box moving to 0.01 mm in front of a
        // wall skipped the wall (their boxes do not overlap), ended up closer than SurfaceEpsilon, and after rounding to
        // float touched the wall and counted as inside it (found on the test map: jumping next to a wall).
        bool DoesWayTouchBrushBounds(const CollisionBrush& brush, const glm::dvec3& start, const glm::dvec3& end,
                                     const glm::dvec3& halfExtents)
        {
            const glm::dvec3 margin = halfExtents + glm::dvec3(SurfaceEpsilon);
            const glm::dvec3 wayMinimum = glm::min(start, end) - margin;
            const glm::dvec3 wayMaximum = glm::max(start, end) + margin;

            // Two boxes overlap when they overlap along every axis.
            return glm::all(glm::lessThanEqual(wayMinimum, brush.bounds.maximum)) &&
                   glm::all(glm::greaterThanEqual(wayMaximum, brush.bounds.minimum));
        }

        // Traces against one brush and updates result if the box hits this brush earlier than everything before.
        // The same algorithm as CM_ClipBoxToBrush of Quake 2.
        void ClipToBrush(const CollisionBrush& brush, const glm::dvec3& start, const glm::dvec3& end,
                         const glm::dvec3& halfExtents, TraceResult& result)
        {
            // Fractions of the way where the line enters and leaves the brush: -1 and 1 mean "not found yet".
            double enterFraction = -1.0;
            double leaveFraction = 1.0;
            const Core::Plane* hitPlane = nullptr;

            bool startsOutside = false; // the start is in front of at least one plane, so it is not inside the brush
            bool endsOutside = false;   // the same for the end

            for (const Core::Plane& plane : brush.planes)
            {
                // Signed distances of the start and the end from the plane moved out for the box: positive in front.
                const double movedDistanceFromOrigin = plane.distanceFromOrigin + CalculateReach(plane.normal, halfExtents);
                const double startDistance = glm::dot(plane.normal, start) - movedDistanceFromOrigin;
                const double endDistance = glm::dot(plane.normal, end) - movedDistanceFromOrigin;

                if (startDistance > 0.0)
                    startsOutside = true;
                if (endDistance > 0.0)
                    endsOutside = true;

                // The whole way is in front of this plane (the end at least SurfaceEpsilon in front, or moving away from
                // it): the brush is behind the plane, so the box never touches the brush.
                if (startDistance > 0.0 && (endDistance >= SurfaceEpsilon || endDistance >= startDistance))
                    return;

                // The whole way is behind this plane: it limits nothing.
                if (startDistance <= 0.0 && endDistance <= 0.0)
                    continue;

                // The line crosses the plane. The distances split the way in the same proportion as the plane does
                // (like in Core::ClipPolygon): startDistance / (startDistance - endDistance).
                if (startDistance > endDistance)
                {
                    // Entering the brush. The stopping point is moved SurfaceEpsilon back, in front of the surface.
                    //
                    // A box that starts closer to the surface than SurfaceEpsilon would have to stop behind its start:
                    // the fraction is negative, and can even be below -1 (start 0.00001 m away, moving 0.001 m into the
                    // wall: (0.00001 - 0.00098) / 0.001 = -0.97, a bit slower and it is below -1). Such a fraction is
                    // made 0 (do not move) before it is compared, as in Quake 3. Compared first, a fraction below -1
                    // lost against the starting enterFraction of -1: the wall was not found, and the box moved into it.
                    // (Found on the test map: jumping next to a wall pushed the player out of it every few ticks.)
                    const double fraction = std::max((startDistance - SurfaceEpsilon) / (startDistance - endDistance), 0.0);
                    if (fraction > enterFraction)
                    {
                        enterFraction = fraction;
                        hitPlane = &plane;
                    }
                }
                else
                {
                    // Leaving the brush.
                    const double fraction = (startDistance + SurfaceEpsilon) / (startDistance - endDistance);
                    leaveFraction = std::min(leaveFraction, fraction);
                }
            }

            if (!startsOutside)
            {
                // Behind every plane at the start: the box begins inside the brush. If the end is inside too, it is stuck.
                result.startsInSolid = true;
                if (!endsOutside)
                {
                    result.isStuck = true;
                    result.fraction = 0.0;
                    result.hitNormal = glm::dvec3(0.0);
                }

                return;
            }

            // A hit if the line enters the brush before it leaves it, and earlier than any hit found before.
            if (hitPlane != nullptr && enterFraction < leaveFraction && enterFraction < result.fraction)
            {
                result.fraction = enterFraction;
                result.hitNormal = hitPlane->normal;
            }
        }
    }

    namespace
    {
        // Clips the way against every brush of the list into result (the earliest hit so far stays). Stops once stuck.
        void ClipToBrushes(std::span<const CollisionBrush> brushes, const glm::dvec3& start, const glm::dvec3& end,
                           const glm::dvec3& halfExtents, TraceResult& result)
        {
            for (const CollisionBrush& brush : brushes)
            {
                if (result.isStuck)
                    return;
                if (DoesWayTouchBrushBounds(brush, start, end, halfExtents))
                    ClipToBrush(brush, start, end, halfExtents, result);
            }
        }
    }

    TraceResult TraceBox(const CollisionWorld& world, const glm::dvec3& start, const glm::dvec3& end,
                         const glm::dvec3& halfExtents)
    {
        // A negative half size would move the planes into the brush instead of out of it. 0 is fine: a point, a ray.
        assert(glm::all(glm::greaterThanEqual(halfExtents, glm::dvec3(0.0))));

        TraceResult result;
        ClipToBrushes(world.level, start, end, halfExtents, result);
        ClipToBrushes(world.characters, start, end, halfExtents, result);
        result.endPosition = start + (end - start) * result.fraction;

        return result;
    }
}
