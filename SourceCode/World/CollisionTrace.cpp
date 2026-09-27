#include "World/CollisionTrace.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>

#include <algorithm>

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
        bool DoesWayTouchBrushBounds(const CollisionBrush& brush, const glm::dvec3& start, const glm::dvec3& end,
                                     const glm::dvec3& halfExtents)
        {
            const glm::dvec3 wayMinimum = glm::min(start, end) - halfExtents;
            const glm::dvec3 wayMaximum = glm::max(start, end) + halfExtents;

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
                const double movedDistance = plane.distance + CalculateReach(plane.normal, halfExtents);
                const double startDistance = glm::dot(plane.normal, start) - movedDistance;
                const double endDistance = glm::dot(plane.normal, end) - movedDistance;

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
                    const double fraction = (startDistance - SurfaceEpsilon) / (startDistance - endDistance);
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
                result.fraction = std::max(enterFraction, 0.0);
                result.hitNormal = hitPlane->normal;
            }
        }
    }

    TraceResult TraceBox(std::span<const CollisionBrush> brushes, const glm::dvec3& start, const glm::dvec3& end,
                         const glm::dvec3& halfExtents)
    {
        TraceResult result;

        for (const CollisionBrush& brush : brushes)
        {
            if (!DoesWayTouchBrushBounds(brush, start, end, halfExtents))
                continue;

            ClipToBrush(brush, start, end, halfExtents, result);
            if (result.isStuck)
                break;
        }

        result.endPosition = start + (end - start) * result.fraction;

        return result;
    }
}
