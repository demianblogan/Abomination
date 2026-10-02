#include "Gameplay/Weapons/Weapon.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <numbers>

namespace Abomination::Gameplay
{
    std::vector<glm::vec3> GeneratePelletDirections(const glm::vec3& forward, const glm::vec3& up, float spreadAngle,
                                                    int count, Core::Random& random)
    {
        // Two directions across the cone, perpendicular to forward and to each other: the right and the up of the shot.
        const glm::vec3 right = glm::normalize(glm::cross(forward, up));
        const glm::vec3 crossUp = glm::cross(right, forward);

        // A point on a disc of radius tan(spreadAngle) one meter in front of the eyes: the pellet flies through it. Its
        // angle around the middle is even over the whole circle. Its distance from the middle is the square root of an
        // even random number: the disc has more area far from the middle (a ring's area grows with its radius), so
        // without the root the pellets would crowd in the middle.
        const float discRadius = std::tan(spreadAngle);

        std::vector<glm::vec3> directions;
        directions.reserve(static_cast<std::size_t>(count));
        for (int pellet = 0; pellet < count; ++pellet)
        {
            const float angle = random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>);
            const float radius = discRadius * std::sqrt(random.GetFloat(0.0f, 1.0f));
            const glm::vec3 pointOnDisc = forward + right * (radius * std::cos(angle)) + crossUp * (radius * std::sin(angle));
            directions.push_back(glm::normalize(pointOnDisc));
        }

        return directions;
    }
}
