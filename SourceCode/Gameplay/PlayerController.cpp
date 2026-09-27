#include "Gameplay/PlayerController.h"

#include "Gameplay/MouseLook.h"
#include "Input/ActionStates.h"
#include "Input/Mouse.h"

namespace Abomination::Gameplay
{
    using Input::Action;

    PlayerController::PlayerController(const PlayerControllerSettings& settings) noexcept
        : m_settings(settings)
    {}

    void PlayerController::UpdateRotation(PlayerLook& look, const Input::ActionStates& actions,
                                          const Input::Mouse& mouse) const
    {
        // Not in the frame LookAroundMode starts: switching the mouse into relative mode can produce one big jump of
        // movement in that frame, which would snap the view.
        const bool isLookingAround =
            actions.IsActionActive(Action::LookAroundMode) && !actions.WasActionStarted(Action::LookAroundMode);
        if (!isLookingAround)
            return;

        TurnByMouse(look.yaw, look.pitch, mouse.GetMovement(), m_settings.mouseSensitivity);
    }
}
