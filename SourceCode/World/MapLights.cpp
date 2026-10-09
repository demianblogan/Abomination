#include "World/MapLights.h"

#include "Core/Math/Units.h"
#include "Renderer/ColorSpace.h"
#include "World/MapCoordinates.h"

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include <string>

namespace Abomination::World
{
    namespace
    {
        // A spot is stronger by default than a point light: it lights a patch far from itself (across a room), where the
        // light of 5 at 1 m is long gone (1/25 of it at 5 m); a point light usually stands by the wall it lights.
        constexpr float DefaultSpotIntensity = 30.0f;

        // A spot points along its local forward (-Z), turned first up or down by the pitch around its right side, then
        // around the vertical by the yaw, like the camera (see Core::Transform). The angles are "pitch yaw roll" as
        // TrenchBroom writes them, with a positive pitch down; the camera turns up by a positive pitch, so it is negated.
        // Without angles, all of them are 0: along +X of the map.
        glm::quat ReadSpotRotation(const MapEntity& entity)
        {
            glm::dvec3 angles(0.0);
            if (const std::string* text = FindProperty(entity, "angles"); text != nullptr)
                angles = ParseVectorProperty(*text).value_or(angles);

            const float pitch = -glm::radians(static_cast<float>(angles.x));
            const float yaw = ConvertMapAngleToYaw(angles.y);

            return glm::angleAxis(yaw, Core::WorldUp) * glm::angleAxis(pitch, Core::LocalRight);
        }
    }

    void ReadLightProperties(const MapEntity& entity, Renderer::Light& light)
    {
        light.intensity = static_cast<float>(ReadNumberProperty(entity, "intensity", light.intensity));
        light.range = static_cast<float>(
            Core::MapUnitsToMeters(ReadNumberProperty(entity, "range", Core::MetersToMapUnits(double{light.range}))));

        // TrenchBroom writes a color as three numbers from 0 to 255, picked on the screen, so in sRGB.
        if (const std::string* colorText = FindProperty(entity, "_color"); colorText != nullptr)
            if (const std::optional<glm::dvec3> color = ParseVectorProperty(*colorText); color.has_value())
                light.color = Renderer::ConvertSRGBToLinear(glm::clamp(glm::vec3(*color / 255.0), 0.0f, 1.0f));
    }

    std::optional<MapLight> ReadMapLight(const MapEntity& entity)
    {
        const std::string* className = FindProperty(entity, "classname");
        if (className == nullptr || (*className != "point_light" && *className != "spot_light"))
            return std::nullopt;

        MapLight result;
        Renderer::Light& light = result.light;

        if (const std::string* origin = FindProperty(entity, "origin"); origin != nullptr)
            if (const std::optional<glm::dvec3> position = ParseVectorProperty(*origin); position.has_value())
                result.transform.position = ConvertMapPosition(*position);

        const bool isSpot = *className == "spot_light";
        if (isSpot)
            light.intensity = DefaultSpotIntensity;
        ReadLightProperties(entity, light);

        if (isSpot)
        {
            light.type = Renderer::LightType::Spot;
            result.transform.rotation = ReadSpotRotation(entity);

            // The inner angle cannot be wider than the outer one: the light would end before it started to fade.
            const double outerDegrees = ReadNumberProperty(entity, "cone", glm::degrees(double{light.outerConeAngle}));
            const double innerDegrees =
                ReadNumberProperty(entity, "inner_cone", glm::degrees(double{light.innerConeAngle}));
            light.outerConeAngle = glm::radians(static_cast<float>(outerDegrees));
            light.innerConeAngle = glm::min(glm::radians(static_cast<float>(innerDegrees)), light.outerConeAngle);
        }

        return result;
    }
}
