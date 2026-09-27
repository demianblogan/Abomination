#pragma once

#include "Core/Transform.h"
#include "Renderer/DebugLines.h"
#include "World/CollisionBrush.h"
#include "World/CollisionTrace.h"
#include "World/PlayerStart.h"

#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace Abomination::World
{
    // What flies from the camera in the cast of the collision tools (like the raycast and box cast of other engines).
    enum class CastShape : std::uint8_t
    {
        Ray,       // no size: a ray, like a bullet
        SmallBox,  // 0.5 m cube
        PlayerBox, // the size of the player
    };

    // Names for the debug overlay, in the order of the enum values.
    inline constexpr std::array<std::string_view, 3> CastShapeNames = {"Ray", "Small box", "Player box"};

    [[nodiscard]] glm::dvec3 GetHalfExtents(CastShape shape);

    // What the collision tools of the debug overlay (View > Physics > Collisions) show. Changed from the overlay.
    struct CollisionDebugSettings
    {
        // The bounding box of every collider (collision brush).
        bool areColliderBoundsVisible = false;

        // A ray or box cast from the camera straight ahead: where it stops and the normal of what it hits.
        bool isCameraCastEnabled = false;
        CastShape cameraCastShape = CastShape::PlayerBox;

        // The free-fly camera slides along walls instead of flying through them (see CameraHalfExtents).
        bool doesCameraCollide = false;
    };

    // The box of the free-fly camera when it collides: small, so it still fits through doors and between crates.
    inline constexpr glm::dvec3 CameraHalfExtents{0.25, 0.25, 0.25};

    // How far the camera cast reaches.
    inline constexpr double CameraCastLength = 30.0;

    // The camera cast of the last frame, for the overlay.
    struct CameraCast
    {
        TraceResult result;

        // How far the box got, in meters: CameraCastLength * result.fraction.
        double distance = 0.0;

        // False while the camera cast is off.
        bool isValid = false;
    };

    // Runs the enabled collision tools and adds their lines: collider bounds (orange) and the camera cast (the box where
    // it stops in yellow, the normal of the surface as a green arrow). camera is the transform the frame is drawn from.
    // Returns the camera cast (not valid if it is off).
    CameraCast UpdateCollisionDebug(std::span<const CollisionBrush> brushes, const CollisionDebugSettings& settings,
                                    const Core::Transform& camera, Renderer::DebugLines& lines);
}
