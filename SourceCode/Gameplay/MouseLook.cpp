#include "Gameplay/MouseLook.h"

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
    namespace
    {
        // The vertical axis of the world, which the yaw turns around, and the view's own right side, which the pitch
        // tilts around.
        constexpr glm::vec3 WorldUp{0.0f, 1.0f, 0.0f};
        constexpr glm::vec3 LocalRight{1.0f, 0.0f, 0.0f};
    }

    glm::quat CalculateCameraRotation(float yaw, float pitch)
    {
        // The pitch quaternion is on the right, so it is applied first: tilt the view up or down around its own right
        // axis while it still looks along -Z, then turn the tilted view around the world vertical axis by the yaw.
        // The other order would turn first and then tilt around a fixed axis, which after turning is no longer the view's
        // side: the view would roll instead of looking up.
        return glm::angleAxis(yaw, WorldUp) * glm::angleAxis(pitch, LocalRight);
    }

    void TurnByMouse(float& yaw, float& pitch, glm::vec2 mouseMovement, float sensitivity)
    {
        // Moving the mouse to the right (+X) must turn right, which is a negative yaw;
        // moving it up (-Y, screen coordinates grow downwards) must look up, which is a positive pitch.
        yaw -= mouseMovement.x * sensitivity;
        pitch = glm::clamp(pitch - mouseMovement.y * sensitivity, -MaxLookPitch, MaxLookPitch);
    }
}
