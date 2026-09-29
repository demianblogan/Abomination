#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

namespace Abomination::Input
{
    // Something the player or the developer wants to do, independent of which key or button does it.
    // Game code asks for actions, never for keys; InputBindings decides which inputs trigger each action,
    // so keys can be rebound (milestone 0.8) and a gamepad added (0.7) without changing game code.
    // New actions are added here when the game needs them.
    enum class Action : std::uint8_t
    {
        // Moving: the player walks with the first four, the free-fly camera uses all of them
        MoveForward,
        MoveBackward,
        MoveLeft,
        MoveRight,
        MoveUp,
        MoveDown,
        MoveFaster,

        // While active, the mouse turns the view even when the cursor is free (the debug overlay is open)
        LookAroundMode,

        // Player
        Fire,
        Jump,

        // Application
        // Closes the game. Escape for now; from 0.8 Escape opens the pause menu, and quitting moves to a button there.
        Quit,

        // Switches between windowed and borderless (from exclusive fullscreen: to windowed).
        ToggleScreenMode,

        // Developer tools
        ToggleDebugOverlay,
        ToggleConsole,

        // Switches between the player and the free-fly camera (a debug "noclip" view that flies through walls).
        ToggleFreeFlyCamera,

        // Not an action: the number of actions above. Must stay the last value.
        Count,
    };

    inline constexpr std::size_t ActionCount = std::to_underlying(Action::Count);
}
