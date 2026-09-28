#include "Core/Math/BoundingBox.h"

#include <glm/common.hpp>

#include <cstddef>

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
}
