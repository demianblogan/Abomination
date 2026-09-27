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
    // The box traced from the camera by the collision tools.
    enum class TraceShape : std::uint8_t
    {
        Point,     // no size: a ray, like a bullet
        SmallBox,  // 0.5 m cube
        PlayerBox, // the size of the player
    };

    // Names for the debug overlay, in the order of the enum values.
    inline constexpr std::array<std::string_view, 3> TraceShapeNames = {"Point", "Small box", "Player box"};

    [[nodiscard]] glm::dvec3 GetHalfExtents(TraceShape shape);

    // What the collision tools of the debug overlay (View > Collision) show. Changed from the overlay.
    struct CollisionDebugSettings
    {
        // The bounding box of every collision brush.
        bool areBrushBoundsVisible = false;

        // A box traced from the camera straight ahead: where it stops and the normal of what it hits.
        bool isCameraTraceEnabled = false;
        TraceShape cameraTraceShape = TraceShape::PlayerBox;

        // The free-fly camera stops at walls instead of flying through them (see CameraHalfExtents).
        bool doesCameraCollide = false;
    };

    // The box of the free-fly camera when it collides: small, so it still fits through doors and between crates.
    inline constexpr glm::dvec3 CameraHalfExtents{0.25, 0.25, 0.25};

    // How far the camera trace reaches.
    inline constexpr double CameraTraceLength = 30.0;

    // The camera trace of the last frame, for the overlay.
    struct CameraTrace
    {
        TraceResult result;

        // How far the box got, in meters: CameraTraceLength * result.fraction.
        double distance = 0.0;

        // False while the camera trace is off.
        bool isValid = false;
    };

    // Runs the enabled collision tools and adds their lines: brush bounds (orange) and the camera trace (the box where
    // it stops in yellow, the normal of the surface as a green arrow). camera is the transform the frame is drawn from.
    // Returns the camera trace (not valid if it is off).
    CameraTrace UpdateCollisionDebug(std::span<const CollisionBrush> brushes, const CollisionDebugSettings& settings,
                                     const Core::Transform& camera, Renderer::DebugLines& lines);
}
