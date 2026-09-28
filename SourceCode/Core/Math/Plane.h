#pragma once

#include <glm/vec3.hpp>

#include <optional>

namespace Abomination::Core
{
    // An infinite flat surface, stored as its normal and its distance from the origin: the points p on the plane are
    // exactly those with dot(normal, p) == distance. The normal (length 1) points to the "front" side.
    // Planes are the faces of brushes: the normal points out of the brush, and the brush is behind all its planes.
    // Doubles, like the map data, so planes built from map points stay exact.
    struct Plane
    {
        glm::dvec3 normal{0.0, 0.0, 1.0};
        double distance = 0.0;
    };

    // The plane through three points, with the normal cross(first - second, third - second): the order of the points
    // decides which side is the front. This is the order of the .map format, where it makes the normal point out of the
    // brush. Returns nothing if the points lie on one line (or two of them are equal): they do not define a plane.
    [[nodiscard]] std::optional<Plane> CreatePlaneFromPoints(const glm::dvec3& first, const glm::dvec3& second,
                                                              const glm::dvec3& third);

    // Which side of the plane the point is on, and how far: positive in front, negative behind, 0 on the plane.
    // The value is the distance from the point to the plane (because the normal has length 1).
    [[nodiscard]] double CalculateSignedDistance(const Plane& plane, const glm::dvec3& point);
}
