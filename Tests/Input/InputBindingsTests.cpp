#include "Input/InputBindings.h"

#include <gtest/gtest.h>

#include <span>

namespace Abomination::Input
{
    TEST(InputBindings, ActionHasNoBindingsInitially)
    {
        const InputBindings bindings;

        EXPECT_TRUE(bindings.GetBindings(Action::MoveForward).empty());
    }

    TEST(InputBindings, ActionCanHaveSeveralBindings)
    {
        InputBindings bindings;

        bindings.Bind(Action::MoveForward, Key::W);
        bindings.Bind(Action::MoveForward, MouseButton::Forward);

        const std::span<const InputBinding> forwardBindings = bindings.GetBindings(Action::MoveForward);
        ASSERT_EQ(forwardBindings.size(), 2u);
        EXPECT_EQ(forwardBindings[0], InputBinding(Key::W));
        EXPECT_EQ(forwardBindings[1], InputBinding(MouseButton::Forward));
    }

    TEST(InputBindings, UnbindRemovesAllBindingsOfAction)
    {
        InputBindings bindings;
        bindings.Bind(Action::MoveForward, Key::W);
        bindings.Bind(Action::MoveBackward, Key::S);

        bindings.Unbind(Action::MoveForward);

        EXPECT_TRUE(bindings.GetBindings(Action::MoveForward).empty());
        EXPECT_EQ(bindings.GetBindings(Action::MoveBackward).size(), 1u);
    }

    TEST(InputBindings, DefaultBindingsUseWASDF1EscapeAndAltEnter)
    {
        const InputBindings bindings = InputBindings::CreateDefault();

        EXPECT_EQ(bindings.GetBindings(Action::MoveForward)[0], InputBinding(Key::W));
        EXPECT_EQ(bindings.GetBindings(Action::MoveDown)[0], InputBinding(Key::Q));
        EXPECT_EQ(bindings.GetBindings(Action::MoveUp)[0], InputBinding(Key::E));
        EXPECT_EQ(bindings.GetBindings(Action::LookAroundMode)[0], InputBinding(MouseButton::Right));
        EXPECT_EQ(bindings.GetBindings(Action::ToggleDebugOverlay)[0], InputBinding(Key::F1));
        EXPECT_EQ(bindings.GetBindings(Action::Quit)[0], InputBinding(Key::Escape));
        EXPECT_EQ(bindings.GetBindings(Action::ToggleConsole)[0], InputBinding(Key::Grave));
        EXPECT_EQ(bindings.GetBindings(Action::ToggleFreeFlyCamera)[0], InputBinding(Key::F2));
        EXPECT_EQ(bindings.GetBindings(Action::Jump)[0], InputBinding(Key::Space));
        EXPECT_EQ(bindings.GetBindings(Action::ToggleScreenMode)[0], InputBinding(KeyCombination{Key::LeftAlt, Key::Enter}));
    }
}
