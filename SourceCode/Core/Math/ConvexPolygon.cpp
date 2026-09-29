#include "Core/Math/ConvexPolygon.h"

#include <cstddef>

namespace Abomination::Core
{
    namespace
    {
        // Where a point is relative to a plane, with PlaneTolerance: On is a separate answer, not a kind of Behind, because
        // a vertex on the plane is kept like a vertex behind it, but an edge from it never crosses the plane (see below).
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

    ConvexPolygon ClipPolygon(const ConvexPolygon& polygon, const Plane& clippingPlane)
    {
        // Walk around the polygon edge by edge, from the current vertex to the next one (the last edge goes back to the
        // first vertex). This is the Sutherland-Hodgman algorithm, simplified for one plane and a convex polygon.
        ConvexPolygon result;
        result.reserve(polygon.size() + 1); // one plane adds at most one vertex to a convex polygon

        for (std::size_t index = 0; index < polygon.size(); ++index)
        {
            const glm::dvec3& current = polygon[index];
            const glm::dvec3& next = polygon[(index + 1) % polygon.size()];
            const PlaneSide currentSide = ClassifyPoint(clippingPlane, current);
            const PlaneSide nextSide = ClassifyPoint(clippingPlane, next);

            // A vertex behind or on the plane stays.
            if (currentSide != PlaneSide::Front)
                result.push_back(current);

            // The edge crosses the plane only if its ends are on opposite sides. An edge from a vertex On the plane to one
            // in front meets the plane exactly at that vertex, which was kept above: a new vertex there would be its
            // duplicate. That is why On is not treated as Behind here.
            const bool crossesPlane = (currentSide == PlaneSide::Front && nextSide == PlaneSide::Behind) ||
                                      (currentSide == PlaneSide::Behind && nextSide == PlaneSide::Front);
            if (!crossesPlane)
                continue;

            // The signed distances of the two ends split the edge in the same proportion as the plane does:
            // distances 30 and -10 put the crossing at 30 / (30 - (-10)) = 3/4 of the way from current to next.
            const double currentDistance = CalculateSignedDistance(clippingPlane, current);
            const double nextDistance = CalculateSignedDistance(clippingPlane, next);
            const double fraction = currentDistance / (currentDistance - nextDistance);
            result.push_back(current + (next - current) * fraction);
        }

        // Fewer than 3 vertices is not a polygon: the plane cut everything away, or only a line or a point is left.
        if (result.size() < 3)
            result.clear();

        return result;
    }
}
