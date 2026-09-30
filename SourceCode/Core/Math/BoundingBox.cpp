#include "Core/Math/BoundingBox.h"

#include <glm/common.hpp>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace Abomination::Core
{
    BoundingBox CalculateBoundingBox(std::span<const glm::dvec3> points)
    {
        if (points.empty())
            return {};

        BoundingBox boundingBox{.minimum = points[0], .maximum = points[0]};

        for (std::size_t i = 1; i < points.size(); ++i)
        {
            boundingBox.minimum = glm::min(boundingBox.minimum, points[i]);
            boundingBox.maximum = glm::max(boundingBox.maximum, points[i]);
        }

        return boundingBox;
    }

    std::optional<double> IntersectRay(const BoundingBox& box, const glm::dvec3& origin, const glm::dvec3& direction,
                                       double maxDistance)
    {
        // The "slab" method. Along each axis the box is a slab between two parallel planes (x = minimum.x and
        // x = maximum.x). The ray is inside that slab between two distances; it is inside the box where it is inside
        // all three slabs at once: after the latest of the three entries and before the earliest of the three exits.
        //
        // Example: a ray from (-5, 0, 0) along +X into a box from (-1, -1, -1) to (1, 1, 1). In the X slab it is from
        // distance 4 to 6; in the Y and Z slabs always (it runs along them). So it is in the box from 4 to 6: it enters
        // at 4.
        double entry = 0.0;
        double exit = maxDistance;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (direction[axis] == 0.0)
            {
                // Parallel to the slab: always inside it, or never.
                if (origin[axis] < box.minimum[axis] || origin[axis] > box.maximum[axis])
                    return std::nullopt;
                continue;
            }

            // The distances at which the ray crosses the two planes of the slab. Going in the negative direction, it
            // meets the maximum plane first, so the two are swapped.
            double nearDistance = (box.minimum[axis] - origin[axis]) / direction[axis];
            double farDistance = (box.maximum[axis] - origin[axis]) / direction[axis];
            if (nearDistance > farDistance)
                std::swap(nearDistance, farDistance);

            entry = std::max(entry, nearDistance);
            exit = std::min(exit, farDistance);
            if (entry > exit)
                return std::nullopt;
        }

        return entry;
    }
}
