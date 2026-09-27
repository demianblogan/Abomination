#pragma once

#include "Gameplay/Player.h"
#include "Physics/CharacterMovement.h"

#include <glm/vec2.hpp>

namespace Abomination::Input
{
    class ActionStates;
}

namespace Abomination::Gameplay
{
    // Values the player tunes (later in the options menu).
    struct PlayerControllerSettings
    {
        // Radians the view turns per pixel of mouse movement (see FreeFlyCameraSettings::mouseSensitivity).
        float mouseSensitivity = 0.0025f;
    };

    // Turns input into what the player does: the mouse turns the view, the movement keys say where the player wants to
    // go. How the player then moves through the level (speed, walls, stairs) is decided by the movement code of the
    // Physics module, which knows nothing about keys; enemies will use the same movement code with an AI instead.
    class PlayerController
    {
    public:
        explicit PlayerController(const PlayerControllerSettings& settings = {}) noexcept;

        // Turns the view by the mouse movement of this frame (pixels). Called once per frame, not in ticks: the view must
        // follow the mouse immediately. Whoever owns the window decides when the mouse turns the view at all and passes
        // no movement otherwise.
        void UpdateRotation(PlayerLook& look, glm::vec2 mouseMovement) const;

        // Remembers the presses of this frame that must not be lost before the next tick. Called once per frame.
        // A tick reads the actions of its frame, but a frame may have no tick at all (at a high frame rate most frames
        // have none): a jump pressed and released in such a frame would never reach a tick. So a press is kept here
        // until the next move command takes it.
        void CollectFrameInput(const Input::ActionStates& actions);

        // What the player wants to do this tick: MoveForward/Backward/Left/Right (WASD) relative to where the player
        // looks, but always horizontal: looking at the floor does not make the player walk into it; and a jump if one
        // was pressed since the last command (see CollectFrameInput), which it takes.
        [[nodiscard]] Physics::MoveCommand CreateMoveCommand(const PlayerLook& look, const Input::ActionStates& actions);

    private:
        PlayerControllerSettings m_settings;

        // A jump was pressed and no move command has taken it yet.
        bool m_isJumpRequested = false;
    };
}
