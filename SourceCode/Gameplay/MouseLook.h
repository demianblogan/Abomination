#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>

// Looking around with the mouse, shared by everything that turns a view with the mouse (the player, the free-fly camera).
namespace Abomination::Gameplay
{
    // Component: where an entity looks, as two angles in radians. No roll: the horizon always stays level.
    //   yaw   - turn left/right around the world up axis (+Y). 0 looks along -Z (OpenGL's "forward");
    //           a positive yaw turns to the LEFT (counter-clockwise when seen from above).
    //   pitch - tilt up/down. 0 looks at the horizon, a positive pitch looks up.
    // Angles are kept instead of only a rotation, because mouse movement adds to them directly and a limit on pitch is
    // easy to apply to an angle. The rotation is built from them (CalculateCameraRotation).
    struct LookAngles
    {
        float yaw = 0.0f;
        float pitch = 0.0f;
    };

    // Pitch is kept within this range. At exactly +-90 degrees the view direction would be parallel to the up axis, and
    // the camera could no longer tell where its right side is: the picture would suddenly flip.
    inline constexpr float MaxLookPitch = glm::radians(89.0f);

    // Radians the view turns per pixel of mouse movement: 0.0025 is about 0.14 degrees, so moving the mouse by 630 pixels
    // turns the view by 90 degrees. The default of every controller that turns a view with the mouse.
    inline constexpr float DefaultMouseSensitivity = 0.0025f;

    // The rotation of a view turned by the angles: first tilt by the pitch around +X, then turn by the yaw around the
    // world +Y. Applied to the forward direction (0, 0, -1), it gives the direction the view looks in.
    [[nodiscard]] glm::quat CalculateCameraRotation(const LookAngles& angles);

    // Changes the angles by the mouse movement of one frame (pixels): moving the mouse right turns right (a smaller yaw),
    // moving it up looks up (a bigger pitch, clamped to MaxLookPitch). sensitivity is radians per pixel.
    // The movement is the distance moved during the frame, so nothing is multiplied by a delta time: the same hand
    // movement turns the view by the same angle at any frame rate.
    void TurnByMouse(LookAngles& angles, glm::vec2 mouseMovement, float sensitivity);
}
