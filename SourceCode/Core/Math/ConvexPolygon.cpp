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
        ConvexPolygon clippedPolygon;
        clippedPolygon.reserve(polygon.size() + 1); // one plane adds at most one vertex to a convex polygon

        for (std::size_t index = 0; index < polygon.size(); ++index)
        {
            // The edge from currentVertex to nextVertex.
            const glm::dvec3& currentVertex = polygon[index];
            const glm::dvec3& nextVertex = polygon[(index + 1) % polygon.size()];
            const PlaneSide currentVertexSide = ClassifyPoint(clippingPlane, currentVertex);
            const PlaneSide nextVertexSide = ClassifyPoint(clippingPlane, nextVertex);

            // A vertex behind or on the plane stays. Only the current vertex is decided here: the next one is decided in
            // the next step, when it becomes the current one.
            if (currentVertexSide != PlaneSide::Front)
                clippedPolygon.push_back(currentVertex);

            // The edge crosses the plane only if its ends are on opposite sides. An edge from a vertex On the plane to one
            // in front meets the plane exactly at that vertex, which was kept above: a new vertex there would be its
            // duplicate. That is why On is not treated as Behind here.
            const bool isEdgeCrossingPlane =
                (currentVertexSide == PlaneSide::Front && nextVertexSide == PlaneSide::Behind) ||
                (currentVertexSide == PlaneSide::Behind && nextVertexSide == PlaneSide::Front);
            if (!isEdgeCrossingPlane)
                continue;

            // Where the edge crosses the plane: walking along the edge, the distance to the plane changes evenly from
            // one end to the other, and the crossing is where it reaches 0. With the distances 30 and -10 it changes by 40
            // over the whole edge, and 30 of them are used up at the crossing: 30 / 40 = 3/4 of the way.
            // crossingFraction is that part of the way from currentVertex to nextVertex, from 0 (at currentVertex) to 1
            // (at nextVertex). The ends are on opposite sides, so the two distances differ and the division is safe.
            const double currentVertexDistanceToPlane = CalculateSignedDistance(clippingPlane, currentVertex);
            const double nextVertexDistanceToPlane = CalculateSignedDistance(clippingPlane, nextVertex);
            const double crossingFraction =
                currentVertexDistanceToPlane / (currentVertexDistanceToPlane - nextVertexDistanceToPlane);

            // Start at currentVertex and walk crossingFraction of the edge (nextVertex - currentVertex is the whole edge).
            const glm::dvec3 crossingVertex = currentVertex + (nextVertex - currentVertex) * crossingFraction;
            clippedPolygon.push_back(crossingVertex);
        }

        // Fewer than 3 vertices is not a polygon: the plane cut everything away, or only a line or a point is left.
        if (clippedPolygon.size() < 3)
            clippedPolygon.clear();

        return clippedPolygon;
    }
}
