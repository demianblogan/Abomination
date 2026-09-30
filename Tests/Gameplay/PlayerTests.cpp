#include "Gameplay/MouseLook.h"
#include "Gameplay/Player.h"
#include "Gameplay/PlayerController.h"
#include "Input/ActionStates.h"
#include "Input/InputBindings.h"
#include "Input/InputDevices.h"
#include "Physics/CharacterBody.h"
#include "Renderer/Camera/CameraLens.h"
#include "World/CollisionDebug.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <initializer_list>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr float Tolerance = 1e-5f;
    }

    TEST(Player, SpawnsAtPlayerStart)
    {
        entt::registry registry;
        const World::PlayerStart start{.boxCenter = {1.0f, 2.0f, 3.0f}, .yaw = 0.5f};

        const entt::entity player = SpawnPlayer(registry, start);

        // The entity is at the center of the box, and the view looks along the yaw of the start.
        EXPECT_EQ(registry.get<Core::Transform>(player).position, start.boxCenter);
        EXPECT_FLOAT_EQ(registry.get<LookAngles>(player).yaw, 0.5f);
        EXPECT_EQ(registry.get<Physics::CharacterBody>(player).halfExtents, World::PlayerHalfExtents);
        EXPECT_TRUE(registry.all_of<Renderer::CameraLens>(player));
    }

    TEST(Player, EyesAreAboveBodyAndLookAlongLookAngles)
    {
        const Core::Transform body{.position = {0.0f, 1.0f, 0.0f}};
        const LookAngles look{.yaw = 0.3f, .pitch = -0.2f};

        const Core::Transform eyes = CalculatePlayerEyeTransform(body, look);

        EXPECT_NEAR(eyes.position.y, 1.0f + PlayerEyeHeight, Tolerance);
        const glm::quat expected = CalculateCameraRotation({0.3f, -0.2f});
        EXPECT_NEAR(glm::dot(eyes.rotation, expected), 1.0f, Tolerance); // the same rotation
    }

    TEST(MouseLook, MouseRightTurnsRightAndPitchIsClamped)
    {
        LookAngles look;

        TurnByMouse(look, {100.0f, -100000.0f}, 0.01f);

        EXPECT_NEAR(look.yaw, -1.0f, Tolerance);   // right is a negative yaw
        EXPECT_FLOAT_EQ(look.pitch, MaxLookPitch); // far up, but not beyond the limit
    }

    TEST(PlayerController, MouseTurnsView)
    {
        const PlayerController controller(PlayerControllerSettings{.mouseSensitivity = 0.01f});
        LookAngles look;

        controller.UpdateRotation(look, {100.0f, 0.0f});

        EXPECT_NEAR(look.yaw, -1.0f, Tolerance);
    }

    class PlayerControllerMoveTest : public ::testing::Test
    {
    protected:
        // The move command while the given keys are held.
        Physics::MoveCommand CreateCommand(std::initializer_list<Input::Key> keys, const LookAngles& look)
        {
            m_devices.keyboard.StartFrame();
            for (const Input::Key key : keys)
                m_devices.keyboard.PressKey(key);
            m_actions.Update(m_devices, m_bindings);

            return m_controller.CreateMoveCommand(look, m_actions);
        }

        Input::InputDevices m_devices;
        Input::InputBindings m_bindings = Input::InputBindings::CreateDefault();
        Input::ActionStates m_actions;
        PlayerController m_controller;
    };

    TEST_F(PlayerControllerMoveTest, ForwardIsWhereTheLookAnglessButHorizontal)
    {
        // Looking down at 45 degrees and straight along -Z: W still goes along -Z, not into the floor.
        const Physics::MoveCommand command = CreateCommand({Input::Key::W}, LookAngles{.yaw = 0.0f, .pitch = -0.78f});

        EXPECT_NEAR(command.wishDirection.x, 0.0f, Tolerance);
        EXPECT_NEAR(command.wishDirection.y, 0.0f, Tolerance);
        EXPECT_NEAR(command.wishDirection.z, -1.0f, Tolerance);
    }

    TEST_F(PlayerControllerMoveTest, DiagonalIsNotFaster)
    {
        const Physics::MoveCommand command = CreateCommand({Input::Key::W, Input::Key::D}, LookAngles{});

        EXPECT_NEAR(glm::length(command.wishDirection), 1.0f, Tolerance);
        EXPECT_GT(command.wishDirection.x, 0.0f); // D goes to the right: +X when looking along -Z
    }

    TEST_F(PlayerControllerMoveTest, NoKeysMeansStanding)
    {
        EXPECT_EQ(CreateCommand({}, LookAngles{}).wishDirection, glm::vec3(0.0f));
    }

    TEST(Player, EyesGlideUpAfterStep)
    {
        StepSmoothing smoothing;

        // The body walks up a step of 0.25 m: the eyes stay behind by that much, minus one tick of catching up.
        UpdateStepSmoothing(smoothing, 0.25f, 1.0f / 60.0f);
        EXPECT_NEAR(smoothing.offset, -0.25f + StepSmoothingSpeed / 60.0f, Tolerance);
        EXPECT_FLOAT_EQ(smoothing.previousOffset, 0.0f);

        // A few ticks later they have caught up and stay there.
        for (int tick = 0; tick < 60; ++tick)
            UpdateStepSmoothing(smoothing, 0.0f, 1.0f / 60.0f);
        EXPECT_FLOAT_EQ(smoothing.offset, 0.0f);
    }

    TEST(Player, EyesFallBehindAtMostMaximumStepLag)
    {
        StepSmoothing smoothing;

        UpdateStepSmoothing(smoothing, 2.0f, 0.0f);

        EXPECT_FLOAT_EQ(smoothing.offset, -MaximumStepLag);
    }

    TEST_F(PlayerControllerMoveTest, JumpPressedBetweenTicksIsNotLost)
    {
        // Frame 1: Space is pressed and released before any tick runs. Frame 2 has a tick: it still gets the jump.
        m_devices.keyboard.StartFrame();
        m_devices.keyboard.PressKey(Input::Key::Space);
        m_actions.Update(m_devices, m_bindings);
        m_controller.CollectFrameInput(m_actions);

        m_devices.keyboard.StartFrame();
        m_devices.keyboard.ReleaseKey(Input::Key::Space);
        m_actions.Update(m_devices, m_bindings);
        m_controller.CollectFrameInput(m_actions);

        EXPECT_TRUE(m_controller.CreateMoveCommand(LookAngles{}, m_actions).wantsToJump);

        // The press is used up: the next tick does not jump again.
        EXPECT_FALSE(m_controller.CreateMoveCommand(LookAngles{}, m_actions).wantsToJump);
    }
}
