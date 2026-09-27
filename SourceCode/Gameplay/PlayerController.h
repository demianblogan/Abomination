#pragma once

#include "Gameplay/Player.h"

namespace Abomination::Input
{
    class ActionStates;
    class Mouse;
}

namespace Abomination::Gameplay
{
    // Values the player tunes (later in the options menu).
    struct PlayerControllerSettings
    {
        // Radians the view turns per pixel of mouse movement (see FreeFlyCameraSettings::mouseSensitivity).
        float mouseSensitivity = 0.0025f;
    };

    // Turns input into what the player does: the mouse turns the view, the movement keys (next steps) say where the player
    // wants to go. How the player then moves through the level (speed, walls, stairs) is decided by the movement code of
    // the Physics module, which knows nothing about keys; enemies will use the same movement code with an AI instead.
    class PlayerController
    {
    public:
        explicit PlayerController(const PlayerControllerSettings& settings = {}) noexcept;

        // Turns the view by the mouse movement of this frame while LookAroundMode is active (not in the frame it starts,
        // like the free-fly camera). Called once per frame, not in ticks: the view must follow the mouse immediately.
        void UpdateRotation(PlayerLook& look, const Input::ActionStates& actions, const Input::Mouse& mouse) const;

    private:
        PlayerControllerSettings m_settings;
    };
}
