#pragma once

#include "Input/Action.h"

#include <array>

namespace Abomination::Input
{
    class InputBindings;
    struct InputDevices;

    // The state of every action for the current frame, calculated from the devices and the bindings:
    //   active  - it is happening now: at least one bound input is held (true every frame while it lasts);
    //   started - it began during this frame (true for exactly one frame);
    //   stopped - it ended during this frame (true for exactly one frame).
    // A toggle such as "show the overlay" uses WasActionStarted(), movement uses IsActionActive().
    class ActionStates
    {
    public:
        // Recalculates all actions. Must be called once per frame, after the devices were filled with the frame's input.
        void Update(const InputDevices& devices, const InputBindings& bindings) noexcept;

        [[nodiscard]] bool IsActionActive(Action action) const noexcept;
        [[nodiscard]] bool WasActionStarted(Action action) const noexcept;
        [[nodiscard]] bool WasActionStopped(Action action) const noexcept;

        // A pair of opposite actions as one axis: 1 while only positive is active, -1 while only negative is, 0 while
        // both or neither are (W and S together cancel out). For moving: GetAxis(MoveForward, MoveBackward).
        [[nodiscard]] float GetAxis(Action positive, Action negative) const noexcept;

    private:
        // The index is the numeric value of Action.
        std::array<bool, ActionCount> m_activeActions{};
        std::array<bool, ActionCount> m_startedActions{};
        std::array<bool, ActionCount> m_stoppedActions{};
    };
}
