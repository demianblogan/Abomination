#pragma once

#include "Core/Math/Plane.h"

#include <glm/vec3.hpp>

#include <vector>

namespace Abomination::Core
{
    // A flat convex polygon in 3D: its corners in order around the edge. Faces of brushes (the convex solids
    // a level is built of, see Plane.h) are such polygons.
    using ConvexPolygon = std::vector<glm::dvec3>;

    // Points closer to a plane than this (in map units) count as lying on it. Map points and the intersections
    // calculated from them carry tiny rounding errors (around 1e-12); without a tolerance a corner lying exactly on
    // a plane could count as a hair in front of it and be cut off, or produce a duplicate corner.
    inline constexpr double PlaneTolerance = 1e-6;

    // Keeps the part of the polygon behind the plane and throws away the part in front of it (the "clipping" step of
    // building brush faces). Corners behind or on the plane are kept; where an edge crosses the plane, a new corner is
    // added at the crossing point. The order of the corners (and so the side the polygon faces) does not change.
    // Returns an empty polygon if nothing is left behind the plane.
    [[nodiscard]] ConvexPolygon ClipPolygon(const ConvexPolygon& polygon, const Plane& plane);
}
