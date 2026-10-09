#include "Renderer/Debug/LightDebugLines.h"

#include "Core/Scene/Transform.h"
#include "Renderer/ColorSpace.h"
#include "Renderer/Light.h"

#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>

#include <cmath>

namespace Abomination::Renderer
{
    namespace
    {
        // Half the size of the star at a light, and the length of the arrow of a spot.
        constexpr float StarHalfSize = 0.15f;
        constexpr float SpotArrowLength = 0.5f;

        // Straight pieces of a circle: enough to look round at the size of a room.
        constexpr int CircleSegmentCount = 32;

        // A circle around center in the plane of two perpendicular unit directions.
        void AddCircle(DebugLines& lines, const glm::vec3& center, const glm::vec3& axisA, const glm::vec3& axisB,
                       float radius, const glm::vec3& color)
        {
            glm::vec3 previous = center + axisA * radius;
            for (int segment = 1; segment <= CircleSegmentCount; ++segment)
            {
                const float angle = glm::two_pi<float>() * static_cast<float>(segment) / CircleSegmentCount;
                const glm::vec3 point = center + (axisA * std::cos(angle) + axisB * std::sin(angle)) * radius;
                lines.AddLine(previous, point, color);
                previous = point;
            }
        }

        // The color of a light as the lines show it: as bright as a line can be, whatever the intensity, and in sRGB like
        // every color the lines are given (the light keeps linear values).
        glm::vec3 CalculateLineColor(const Light& light)
        {
            const float brightest = glm::max(light.color.r, glm::max(light.color.g, light.color.b));
            const glm::vec3 color = brightest > 0.0f ? light.color / brightest : glm::vec3(1.0f);

            return ConvertLinearToSRGB(color);
        }
    }

    void AddLightDebugLines(const entt::registry& registry, DebugLines& lines)
    {
        constexpr auto AlwaysVisible = DebugLineDepth::AlwaysVisible;
        constexpr glm::vec3 AxisX(1.0f, 0.0f, 0.0f);
        constexpr glm::vec3 AxisY(0.0f, 1.0f, 0.0f);
        constexpr glm::vec3 AxisZ(0.0f, 0.0f, 1.0f);

        for (const auto [entity, transform, light] : registry.view<const Core::Transform, const Light>().each())
        {
            const glm::vec3& position = transform.position;
            const glm::vec3 color = CalculateLineColor(light);

            for (const glm::vec3& axis : {AxisX, AxisY, AxisZ})
                lines.AddLine(position - axis * StarHalfSize, position + axis * StarHalfSize, color, AlwaysVisible);

            if (light.type == LightType::Point)
            {
                AddCircle(lines, position, AxisX, AxisY, light.range, color);
                AddCircle(lines, position, AxisY, AxisZ, light.range, color);
                AddCircle(lines, position, AxisZ, AxisX, light.range, color);
                continue;
            }

            // A spot: its axis, and the cone as the circle where its light ends (range along the edge of the cone), with
            // four lines from the light to it.
            const glm::vec3 forward = transform.rotation * Core::LocalForward;
            const glm::vec3 right = transform.rotation * Core::LocalRight;
            const glm::vec3 up = glm::cross(right, forward);
            lines.AddArrow(position, position + forward * SpotArrowLength, color, AlwaysVisible);

            const glm::vec3 circleCenter = position + forward * (light.range * std::cos(light.outerConeAngle));
            const float circleRadius = light.range * std::sin(light.outerConeAngle);
            AddCircle(lines, circleCenter, right, up, circleRadius, color);
            for (const glm::vec3& side : {right, up, -right, -up})
                lines.AddLine(position, circleCenter + side * circleRadius, color);
        }
    }
}
