#pragma once

#include "Core/Math/Plane.h"

#include <glm/vec3.hpp>

#include <vector>

namespace Abomination::Core
{
    // A flat convex polygon in 3D: its vertices (the points where its edges meet) in order around the edge; neighbours
    // in the list are joined by an edge, and the last vertex by an edge to the first. All vertices lie in one plane.
    // Convex: without dents, any line between two of its points stays inside it. Faces of brushes (the convex solids
    // a level is built of, see Plane.h) are such polygons.
    using ConvexPolygon = std::vector<glm::dvec3>;

    // The tolerance of plane tests: points closer to a plane than this (in map units) count as lying on it, like a part
    // of "10 mm +- 0.01 mm" counts as 10 mm. Map points and the intersections calculated from them carry tiny rounding
    // errors (around 1e-12); without a tolerance a vertex lying exactly on a plane could count as a hair in front of it
    // and be cut off, or an edge could "cross" the plane a hair away from that vertex and add a duplicate vertex there.
    // Real vertices of a map are whole units away from the planes they do not lie on, so the tolerance changes nothing
    // real.
    inline constexpr double PlaneTolerance = 1e-6;

    // Cuts one polygon (for example a face of a brush while it is being built) with one plane and returns the part
    // behind that plane: a new list of vertices. "Behind" is decided by the normal of clippingPlane, not by where most
    // of the polygon lies: the side the normal points to is the front and is thrown away, the other side is kept. For
    // brushes that is exactly the inside, because the normals of their planes point out of them.
    //
    // Vertices behind or on the plane are kept; where an edge crosses the plane, a new vertex is added at the crossing
    // point. The order of the vertices (and so the side the polygon faces) does not change. Returns an empty polygon
    // if nothing is left behind the plane. The polygon does not have to lie in any particular relation to the plane:
    // BuildBrushPolygons cuts a face of a brush with the planes of the other faces, one call per plane.
    [[nodiscard]] ConvexPolygon ClipPolygon(const ConvexPolygon& polygon, const Plane& clippingPlane);
}
