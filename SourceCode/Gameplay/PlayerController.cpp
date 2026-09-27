#include "Gameplay/PlayerController.h"

#include "Gameplay/MouseLook.h"
#include "Input/ActionStates.h"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
    using Input::Action;

    namespace
    {
        // 1.0 while the action is active, 0.0 otherwise: lets opposite actions cancel each other out by subtraction.
        float GetActionValue(const Input::ActionStates& actions, Action action) noexcept
        {
            return actions.IsActionActive(action) ? 1.0f : 0.0f;
        }
    }

    PlayerController::PlayerController(const PlayerControllerSettings& settings) noexcept
        : m_settings(settings)
    {}

    void PlayerController::UpdateRotation(PlayerLook& look, glm::vec2 mouseMovement) const
    {
        TurnByMouse(look.yaw, look.pitch, mouseMovement, m_settings.mouseSensitivity);
    }

    Physics::MoveCommand PlayerController::CreateMoveCommand(const PlayerLook& look,
                                                             const Input::ActionStates& actions) const
    {
        const float forwardInput =
            GetActionValue(actions, Action::MoveForward) - GetActionValue(actions, Action::MoveBackward);
        const float rightInput = GetActionValue(actions, Action::MoveRight) - GetActionValue(actions, Action::MoveLeft);

        // Forward and right on the ground, from the yaw only. A yaw of 0 looks along -Z, and a positive yaw turns left
        // (see MouseLook.h): forward is (-sin yaw, 0, -cos yaw), right is forward turned 90 degrees to the right.
        const glm::vec3 forward(-glm::sin(look.yaw), 0.0f, -glm::cos(look.yaw));
        const glm::vec3 right(glm::cos(look.yaw), 0.0f, -glm::sin(look.yaw));

        glm::vec3 direction = forward * forwardInput + right * rightInput;

        // W + D together must not be faster than W alone: the length is made 1 in every direction.
        if (direction != glm::vec3(0.0f))
            direction = glm::normalize(direction);

        return Physics::MoveCommand{.wishDirection = direction};
    }
}
