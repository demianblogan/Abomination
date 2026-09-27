#pragma once

#include <glm/vec3.hpp>

#include <span>

namespace Abomination::Core
{
    // An axis-aligned bounding box (AABB): the smallest box with sides along the X, Y and Z axes that contains something.
    // Two corners describe it: the one with the smallest coordinates and the one with the largest.
    //
    // Checking two such boxes against each other takes six comparisons, so collision code first compares boxes and does
    // the exact (slower) check only for objects whose boxes touch.
    struct BoundingBox
    {
        glm::dvec3 minimum{0.0};
        glm::dvec3 maximum{0.0};
    };

    // The bounding box of the points: minimum is the smallest x, the smallest y and the smallest z among all points,
    // maximum the largest ones (each axis on its own, so the corners do not have to be points of the list).
    // No points gives a box with both corners at (0, 0, 0).
    [[nodiscard]] BoundingBox CalculateBoundingBox(std::span<const glm::dvec3> points);
}
