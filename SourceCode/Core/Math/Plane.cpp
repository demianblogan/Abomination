#include "Core/Math/Plane.h"

#include <glm/geometric.hpp>

namespace Abomination::Core
{
    namespace
    {
        // A cross product shorter than this means the three points (practically) lie on one line. Map coordinates are
        // at least 1 unit apart in practice, so real faces give cross products many orders of magnitude longer.
        constexpr double SmallestCrossProductLength = 1e-9;
    }

    std::optional<Plane> CreatePlaneFromPoints(const glm::dvec3& first, const glm::dvec3& second, const glm::dvec3& third)
    {
        // Two edges from the second point; their cross product is perpendicular to both, so to the plane.
        // Its length is the area of the parallelogram they span: zero when the points lie on one line.
        const glm::dvec3 cross = glm::cross(first - second, third - second);
        const double crossLength = glm::length(cross);
        if (crossLength < SmallestCrossProductLength)
            return std::nullopt;

        Plane plane;
        plane.normal = cross / crossLength;

        // The first point lies on the plane, so dot(normal, first) is the plane's distance from the origin.
        plane.distance = glm::dot(plane.normal, first);

        return plane;
    }

    double CalculateSignedDistance(const Plane& plane, const glm::dvec3& point)
    {
        return glm::dot(plane.normal, point) - plane.distance;
    }
}
