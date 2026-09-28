#include "Core/Math/ConvexPolygon.h"

#include <cstddef>

namespace Abomination::Core
{
    namespace
    {
        enum class PlaneSide
        {
            Front,
            Behind,
            On,
        };

        PlaneSide ClassifyPoint(const Plane& plane, const glm::dvec3& point)
        {
            const double signedDistance = CalculateSignedDistance(plane, point);
            if (signedDistance > PlaneTolerance)
                return PlaneSide::Front;
            if (signedDistance < -PlaneTolerance)
                return PlaneSide::Behind;

            return PlaneSide::On;
        }
    }

    ConvexPolygon ClipPolygon(const ConvexPolygon& polygon, const Plane& plane)
    {
        // Walk around the polygon edge by edge, from the current corner to the next one (the last edge goes back to the
        // first corner). This is the Sutherland-Hodgman algorithm, simplified for one plane and a convex polygon.
        ConvexPolygon result;
        result.reserve(polygon.size() + 1); // one plane adds at most one corner to a convex polygon

        for (std::size_t index = 0; index < polygon.size(); ++index)
        {
            const glm::dvec3& current = polygon[index];
            const glm::dvec3& next = polygon[(index + 1) % polygon.size()];
            const PlaneSide currentSide = ClassifyPoint(plane, current);
            const PlaneSide nextSide = ClassifyPoint(plane, next);

            // A corner behind or on the plane stays.
            if (currentSide != PlaneSide::Front)
                result.push_back(current);

            // The edge crosses the plane only if its ends are on opposite sides. A corner lying on the plane was kept
            // above, so it needs no new corner.
            const bool crossesPlane = (currentSide == PlaneSide::Front && nextSide == PlaneSide::Behind) ||
                                      (currentSide == PlaneSide::Behind && nextSide == PlaneSide::Front);
            if (!crossesPlane)
                continue;

            // The signed distances of the two ends split the edge in the same proportion as the plane does:
            // distances 30 and -10 put the crossing at 30 / (30 - (-10)) = 3/4 of the way from current to next.
            const double currentDistance = CalculateSignedDistance(plane, current);
            const double nextDistance = CalculateSignedDistance(plane, next);
            const double fraction = currentDistance / (currentDistance - nextDistance);
            result.push_back(current + (next - current) * fraction);
        }

        // Fewer than 3 corners is not a polygon: the plane cut everything away, or only a line or a point is left.
        if (result.size() < 3)
            result.clear();

        return result;
    }
}
