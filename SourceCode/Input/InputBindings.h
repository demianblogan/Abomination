#pragma once

#include "Input/Action.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"

#include <array>
#include <span>
#include <variant>
#include <vector>

namespace Abomination::Input
{
    // A key pressed while a modifier key is held, like Alt+Enter. The modifier alone or the key alone does nothing.
    struct KeyCombination
    {
        Key modifier;
        Key key;

        // = default compares the members one by one; needed to compare bindings (std::variant compares its contents).
        bool operator==(const KeyCombination&) const = default;
    };

    // One physical input that can trigger an action: a key, a key combination or a mouse button (a gamepad button
    // later). std::variant holds exactly one of the listed types at a time and remembers which one.
    using InputBinding = std::variant<Key, KeyCombination, MouseButton>;

    // Which inputs trigger which action. An action may have several bindings (for example W and the Up arrow);
    // it is active while any of them is held.
    class InputBindings
    {
    public:
        // The default controls: WASD to move, Q/E down/up, Shift faster, the right mouse button to look around,
        // F1 for the debug overlay, F2 for the free-fly camera, ` for the console, Escape to quit, Alt+Enter to switch
        // between windowed and borderless.
        [[nodiscard]] static InputBindings CreateDefault();

        // Adds one more input for the action; the existing bindings of the action stay.
        void Bind(Action action, InputBinding binding);

        // Removes all bindings of the action.
        void Unbind(Action action);

        [[nodiscard]] std::span<const InputBinding> GetBindings(Action action) const noexcept;

    private:
        // The index is the numeric value of Action.
        std::array<std::vector<InputBinding>, ActionCount> m_bindings;
    };
}
