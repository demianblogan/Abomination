#include "World/CollisionDebug.h"

#include <glm/gtc/quaternion.hpp>

#include <array>

namespace Abomination::World
{
    namespace
    {
        constexpr glm::vec3 BoundsColor{1.0f, 0.55f, 0.1f};
        constexpr glm::vec3 HitBoxColor{1.0f, 0.9f, 0.2f};
        constexpr glm::vec3 FreeBoxColor{0.5f, 0.8f, 1.0f};
        constexpr glm::vec3 NormalColor{0.3f, 1.0f, 0.3f};

        // The half size of the small cast box: a cube of 0.5 m, smaller than the player.
        constexpr glm::dvec3 SmallBoxHalfExtents{0.25};

        // The length of the normal arrow in meters.
        constexpr float NormalArrowLength = 0.75f;

        // A point has no size, so it gets a small cross to be visible.
        constexpr float PointMarkerSize = 0.1f;
        constexpr std::array<glm::vec3, 3> WorldAxes = {
            glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};

        void AddCastBox(Renderer::DebugLines& lines, const glm::vec3& center, const glm::vec3& halfExtents,
                         const glm::vec3& color)
        {
            if (halfExtents.x > 0.0f)
            {
                lines.AddBox(center - halfExtents, center + halfExtents, color);
                return;
            }

            for (const glm::vec3& axis : WorldAxes)
                lines.AddLine(center - axis * PointMarkerSize, center + axis * PointMarkerSize, color);
        }
    }

    glm::dvec3 GetHalfExtents(CastShape shape)
    {
        switch (shape)
        {
            case CastShape::Ray:
                return glm::dvec3(0.0);
            case CastShape::SmallBox:
                return SmallBoxHalfExtents;
            case CastShape::PlayerBox:
                return PlayerHalfExtents;
        }

        return glm::dvec3(0.0);
    }

    CameraCast UpdateCollisionDebug(std::span<const CollisionBrush> brushes, const CollisionDebugSettings& settings,
                                    const Core::Transform& camera, Renderer::DebugLines& lines)
    {
        if (settings.areColliderBoundsVisible)
            for (const CollisionBrush& brush : brushes)
                lines.AddBox(glm::vec3(brush.bounds.minimum), glm::vec3(brush.bounds.maximum), BoundsColor);

        CameraCast cast;
        if (!settings.isCameraCastEnabled)
            return cast;

        // The box starts at the camera and flies straight ahead, where the camera looks.
        const glm::dvec3 halfExtents = GetHalfExtents(settings.cameraCastShape);
        const glm::dvec3 start(camera.position);
        const glm::dvec3 forward(camera.rotation * Core::LocalForward);
        cast.result = TraceBox(brushes, start, start + forward * CameraCastLength, halfExtents);
        cast.distance = CameraCastLength * cast.result.fraction;
        cast.isValid = true;

        // Where the box stopped: yellow if it hit something, light blue if it flew the whole way.
        const bool hasHit = cast.result.fraction < 1.0;
        const glm::vec3 stop(cast.result.endPosition);
        AddCastBox(lines, stop, glm::vec3(halfExtents), hasHit ? HitBoxColor : FreeBoxColor);
        if (hasHit)
            lines.AddArrow(stop, stop + glm::vec3(cast.result.hitNormal) * NormalArrowLength, NormalColor);

        return cast;
    }
}
