#include "Input/InputBindings.h"

#include <utility>

namespace Abomination::Input
{
    InputBindings InputBindings::CreateDefault()
    {
        InputBindings bindings;

        bindings.Bind(Action::MoveForward, Key::W);
        bindings.Bind(Action::MoveBackward, Key::S);
        bindings.Bind(Action::MoveLeft, Key::A);
        bindings.Bind(Action::MoveRight, Key::D);
        bindings.Bind(Action::MoveDown, Key::Q);
        bindings.Bind(Action::MoveUp, Key::E);
        bindings.Bind(Action::MoveFaster, Key::LeftShift);
        bindings.Bind(Action::LookAroundMode, MouseButton::Right);
        bindings.Bind(Action::Quit, Key::Escape);
        bindings.Bind(Action::ToggleScreenMode, KeyCombination{.modifier = Key::LeftAlt, .key = Key::Enter});
        bindings.Bind(Action::ToggleScreenMode, KeyCombination{.modifier = Key::RightAlt, .key = Key::Enter});
        bindings.Bind(Action::ToggleDebugOverlay, Key::F1);
        bindings.Bind(Action::ToggleConsole, Key::Grave);

        return bindings;
    }

    void InputBindings::Bind(Action action, InputBinding binding)
    {
        m_bindings[std::to_underlying(action)].push_back(binding);
    }

    void InputBindings::Unbind(Action action)
    {
        m_bindings[std::to_underlying(action)].clear();
    }

    std::span<const InputBinding> InputBindings::GetBindings(Action action) const noexcept
    {
        return m_bindings[std::to_underlying(action)];
    }
}
