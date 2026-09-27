#include "Input/ActionStates.h"

#include "Input/InputBindings.h"
#include "Input/InputDevices.h"

#include <cstddef>
#include <utility>
#include <variant>

namespace Abomination::Input
{
    namespace
    {
        // True if the input is held now, or was pressed during this frame. The second part matters for a very quick
        // tap: a key pressed and released between two frames is no longer held, but the press still has to count.
        bool IsBindingActive(const InputBinding& binding, const InputDevices& devices) noexcept
        {
            if (const Key* key = std::get_if<Key>(&binding); key != nullptr)
                return devices.keyboard.IsKeyHeld(*key) || devices.keyboard.WasKeyPressed(*key);

            // The modifier only has to be held; the key starts the action. Holding Alt and then pressing Enter works, and so
            // does pressing both in the same frame (a press also marks the key as held).
            if (const KeyCombination* combination = std::get_if<KeyCombination>(&binding); combination != nullptr)
                return devices.keyboard.IsKeyHeld(combination->modifier) &&
                       (devices.keyboard.IsKeyHeld(combination->key) || devices.keyboard.WasKeyPressed(combination->key));

            const MouseButton button = std::get<MouseButton>(binding);
            return devices.mouse.IsButtonHeld(button) || devices.mouse.WasButtonPressed(button);
        }

        bool IsAnyBindingActive(Action action, const InputDevices& devices, const InputBindings& bindings) noexcept
        {
            for (const InputBinding& binding : bindings.GetBindings(action))
                if (IsBindingActive(binding, devices))
                    return true;

            return false;
        }
    }

    void ActionStates::Update(const InputDevices& devices, const InputBindings& bindings) noexcept
    {
        for (std::size_t index = 0; index < ActionCount; ++index)
        {
            const auto action = static_cast<Action>(index);
            const bool wasActive = m_activeActions[index];
            const bool isActive = IsAnyBindingActive(action, devices, bindings);

            // Started and stopped are found by comparing with the previous frame, not by looking at "pressed" flags of
            // the inputs: with two bindings (W and Up) pressing the second one while the first is held must not start
            // the action again.
            m_activeActions[index] = isActive;
            m_startedActions[index] = isActive && !wasActive;
            m_stoppedActions[index] = !isActive && wasActive;
        }
    }

    bool ActionStates::IsActionActive(Action action) const noexcept
    {
        return m_activeActions[std::to_underlying(action)];
    }

    bool ActionStates::WasActionStarted(Action action) const noexcept
    {
        return m_startedActions[std::to_underlying(action)];
    }

    bool ActionStates::WasActionStopped(Action action) const noexcept
    {
        return m_stoppedActions[std::to_underlying(action)];
    }

    float ActionStates::GetAxis(Action positive, Action negative) const noexcept
    {
        // Each action counts as 1 while active and 0 otherwise; subtracting lets opposite actions cancel each other out.
        const float positiveValue = IsActionActive(positive) ? 1.0f : 0.0f;
        const float negativeValue = IsActionActive(negative) ? 1.0f : 0.0f;

        return positiveValue - negativeValue;
    }
}
