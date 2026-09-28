#include "World/MapCoordinates.h"

#include "Core/Math/Units.h"

#include <glm/trigonometric.hpp>

#include <charconv>
#include <system_error>

namespace Abomination::World
{
    namespace
    {
        // The turn of the axes shared by all conversions: (x, y, z) -> (x, z, -y).
        glm::dvec3 TurnMapAxes(const glm::dvec3& mapVector)
        {
            return {mapVector.x, mapVector.z, -mapVector.y};
        }
    }

    glm::vec3 ConvertMapPosition(const glm::dvec3& mapPosition)
    {
        return glm::vec3(ConvertMapPositionPrecise(mapPosition));
    }

    glm::dvec3 ConvertMapPositionPrecise(const glm::dvec3& mapPosition)
    {
        return TurnMapAxes(mapPosition / Core::UnitsPerMeter);
    }

    Core::Plane ConvertMapPlane(const Core::Plane& mapPlane)
    {
        return {.normal = TurnMapAxes(mapPlane.normal), .distance = Core::MapUnitsToMeters(mapPlane.distance)};
    }

    float ConvertMapAngleToYaw(double mapAngleDegrees)
    {
        // The direction of a map angle is (cos a, sin a, 0), which becomes (cos a, 0, -sin a) in the game. The camera
        // looks along (-sin yaw, 0, -cos yaw); both are equal when yaw = a - 90 degrees.
        return glm::radians(static_cast<float>(mapAngleDegrees - 90.0));
    }

    std::optional<glm::dvec3> ParseVectorProperty(std::string_view text)
    {
        glm::dvec3 vector(0.0);
        const char* position = text.data();
        const char* end = text.data() + text.size();
        for (glm::length_t component = 0; component < glm::dvec3::length(); ++component)
        {
            while (position < end && *position == ' ')
                ++position;

            const std::from_chars_result result = std::from_chars(position, end, vector[component]);
            if (result.ec != std::errc())
                return std::nullopt;

            position = result.ptr;
        }

        return vector;
    }
}
