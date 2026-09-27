#include "Input/ActionStates.h"
#include "Input/InputBindings.h"
#include "Input/InputDevices.h"

#include <gtest/gtest.h>

namespace Abomination::Input
{
    // Simulates frames: every test step starts a new frame of the devices, applies some input and updates the actions.
    class ActionStatesTest : public ::testing::Test
    {
    protected:
        void StartFrame()
        {
            m_devices.keyboard.StartFrame();
            m_devices.mouse.StartFrame();
        }

        void UpdateActions()
        {
            m_actions.Update(m_devices, m_bindings);
        }

        InputDevices m_devices;
        InputBindings m_bindings = InputBindings::CreateDefault();
        ActionStates m_actions;
    };

    TEST_F(ActionStatesTest, ActionStartsAndIsActiveWhenBoundKeyIsPressed)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Key::W);
        UpdateActions();

        EXPECT_TRUE(m_actions.WasActionStarted(Action::MoveForward));
        EXPECT_TRUE(m_actions.IsActionActive(Action::MoveForward));
        EXPECT_FALSE(m_actions.IsActionActive(Action::MoveBackward));
    }

    TEST_F(ActionStatesTest, StartLastsOneFrameWhileActionStaysActive)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Key::W);
        UpdateActions();

        StartFrame();
        UpdateActions();

        EXPECT_FALSE(m_actions.WasActionStarted(Action::MoveForward));
        EXPECT_TRUE(m_actions.IsActionActive(Action::MoveForward));
    }

    TEST_F(ActionStatesTest, ActionStopsWhenBoundKeyIsReleased)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Key::W);
        UpdateActions();

        StartFrame();
        m_devices.keyboard.ReleaseKey(Key::W);
        UpdateActions();

        EXPECT_FALSE(m_actions.IsActionActive(Action::MoveForward));
        EXPECT_TRUE(m_actions.WasActionStopped(Action::MoveForward));
    }

    TEST_F(ActionStatesTest, MouseButtonBindingTriggersAction)
    {
        StartFrame();
        m_devices.mouse.PressButton(MouseButton::Right);
        UpdateActions();

        EXPECT_TRUE(m_actions.WasActionStarted(Action::LookAroundMode));
    }

    TEST_F(ActionStatesTest, SecondBindingDoesNotStartActiveActionAgain)
    {
        m_bindings.Bind(Action::MoveForward, MouseButton::Forward);
        StartFrame();
        m_devices.keyboard.PressKey(Key::W);
        UpdateActions();

        StartFrame();
        m_devices.mouse.PressButton(MouseButton::Forward);
        UpdateActions();

        EXPECT_FALSE(m_actions.WasActionStarted(Action::MoveForward));
        EXPECT_TRUE(m_actions.IsActionActive(Action::MoveForward));
    }

    TEST_F(ActionStatesTest, QuickTapWithinOneFrameStillStartsAction)
    {
        // The key goes down and up again between two frames: at the moment of the update it is no longer held.
        StartFrame();
        m_devices.keyboard.PressKey(Key::F1);
        m_devices.keyboard.ReleaseKey(Key::F1);
        UpdateActions();

        EXPECT_TRUE(m_actions.WasActionStarted(Action::ToggleDebugOverlay));

        StartFrame();
        UpdateActions();

        EXPECT_TRUE(m_actions.WasActionStopped(Action::ToggleDebugOverlay));
    }

    TEST_F(ActionStatesTest, KeyCombinationStartsActionWhenKeyIsPressedWithModifierHeld)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Key::LeftAlt);
        UpdateActions();

        EXPECT_FALSE(m_actions.IsActionActive(Action::ToggleScreenMode)); // the modifier alone does nothing

        StartFrame();
        m_devices.keyboard.PressKey(Key::Enter);
        UpdateActions();

        EXPECT_TRUE(m_actions.WasActionStarted(Action::ToggleScreenMode));
    }

    TEST_F(ActionStatesTest, KeyCombinationWorksWhenBothKeysArePressedInOneFrame)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Key::RightAlt);
        m_devices.keyboard.PressKey(Key::Enter);
        UpdateActions();

        EXPECT_TRUE(m_actions.WasActionStarted(Action::ToggleScreenMode));
    }

    TEST_F(ActionStatesTest, KeyOfCombinationAloneDoesNothing)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Key::Enter);
        UpdateActions();

        EXPECT_FALSE(m_actions.IsActionActive(Action::ToggleScreenMode));
    }

    TEST_F(ActionStatesTest, OppositeActionsMakeAnAxis)
    {
        StartFrame();
        UpdateActions();
        EXPECT_FLOAT_EQ(m_actions.GetAxis(Action::MoveForward, Action::MoveBackward), 0.0f);

        StartFrame();
        m_devices.keyboard.PressKey(Key::S);
        UpdateActions();
        EXPECT_FLOAT_EQ(m_actions.GetAxis(Action::MoveForward, Action::MoveBackward), -1.0f);

        // W and S together cancel out.
        StartFrame();
        m_devices.keyboard.PressKey(Key::W);
        UpdateActions();
        EXPECT_FLOAT_EQ(m_actions.GetAxis(Action::MoveForward, Action::MoveBackward), 0.0f);

        StartFrame();
        m_devices.keyboard.ReleaseKey(Key::S);
        UpdateActions();
        EXPECT_FLOAT_EQ(m_actions.GetAxis(Action::MoveForward, Action::MoveBackward), 1.0f);
    }
}
