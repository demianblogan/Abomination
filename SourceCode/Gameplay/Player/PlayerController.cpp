#include "Gameplay/Player/PlayerController.h"

#include "Gameplay/Camera/MouseLook.h"
#include "Input/ActionStates.h"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <utility>

namespace Abomination::Gameplay
{
    using Input::Action;

    PlayerController::PlayerController(const PlayerControllerSettings& settings) noexcept
        : m_settings(settings)
    {}

    void PlayerController::UpdateRotation(LookAngles& look, glm::vec2 mouseMovement) const
    {
        TurnByMouse(look, mouseMovement, m_settings.mouseSensitivity);
    }

    void PlayerController::CollectFrameInput(const Input::ActionStates& actions)
    {
        if (actions.WasActionStarted(Action::Jump))
            m_isJumpRequested = true;
    }

    Physics::MoveCommand PlayerController::CreateMoveCommand(const LookAngles& look, const Input::ActionStates& actions)
    {
        const float forwardInput = actions.GetAxis(Action::MoveForward, Action::MoveBackward);
        const float rightInput = actions.GetAxis(Action::MoveRight, Action::MoveLeft);

        // Forward and right on the ground, from the yaw only. A yaw of 0 looks along -Z, and a positive yaw turns left
        // (see MouseLook.h): forward is (-sin yaw, 0, -cos yaw), right is forward turned 90 degrees to the right.
        const glm::vec3 forward(-glm::sin(look.yaw), 0.0f, -glm::cos(look.yaw));
        const glm::vec3 right(glm::cos(look.yaw), 0.0f, -glm::sin(look.yaw));

        glm::vec3 direction = forward * forwardInput + right * rightInput;

        // W + D together must not be faster than W alone: the length is made 1 in every direction.
        if (direction != glm::vec3(0.0f))
            direction = glm::normalize(direction);

        // The jump press goes to this command only: holding the key does not jump again on landing, it has to be pressed
        // again, like in Quake.
        const bool wantsToJump = std::exchange(m_isJumpRequested, false);

        return Physics::MoveCommand{.wishDirection = direction, .wantsToJump = wantsToJump};
    }
}
