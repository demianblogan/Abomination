#include "Gameplay/Camera/MouseLook.h"

#include "Core/Scene/Transform.h"

#include <glm/common.hpp>

namespace Abomination::Gameplay
{
    glm::quat CalculateCameraRotation(const LookAngles& angles)
    {
        // The yaw turns around the vertical axis of the world, the pitch tilts around the view's own right side.
        // The pitch quaternion is on the right, so it is applied first: tilt the view up or down around its own right
        // axis while it still looks along -Z, then turn the tilted view around the world vertical axis by the yaw.
        // The other order would turn first and then tilt around a fixed axis, which after turning is no longer the view's
        // side: the view would roll instead of looking up.
        return glm::angleAxis(angles.yaw, Core::WorldUp) * glm::angleAxis(angles.pitch, Core::LocalRight);
    }

    void TurnByMouse(LookAngles& angles, glm::vec2 mouseMovement, float sensitivity)
    {
        // Moving the mouse to the right (+X) must turn right, which is a negative yaw;
        // moving it up (-Y, screen coordinates grow downwards) must look up, which is a positive pitch.
        angles.yaw -= mouseMovement.x * sensitivity;
        angles.pitch = glm::clamp(angles.pitch - mouseMovement.y * sensitivity, -MaxLookPitch, MaxLookPitch);
    }
}
