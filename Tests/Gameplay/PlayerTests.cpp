#include "Gameplay/MouseLook.h"
#include "Gameplay/Player.h"
#include "Gameplay/PlayerController.h"
#include "Input/ActionStates.h"
#include "Input/InputBindings.h"
#include "Input/InputDevices.h"
#include "Physics/CharacterBody.h"
#include "Renderer/CameraLens.h"
#include "World/CollisionDebug.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr float Tolerance = 1e-5f;
    }

    TEST(Player, SpawnsWithEyesAtPlayerStart)
    {
        entt::registry registry;
        const World::PlayerStart start{.eyePosition = {1.0f, 2.0f, 3.0f}, .yaw = 0.5f};

        const entt::entity player = SpawnPlayer(registry, start);

        // The center of the box is PlayerEyeHeight below the eyes, and the view looks along the yaw of the start.
        const Core::Transform& transform = registry.get<Core::Transform>(player);
        EXPECT_NEAR(transform.position.y, 2.0f - PlayerEyeHeight, Tolerance);
        EXPECT_FLOAT_EQ(registry.get<PlayerLook>(player).yaw, 0.5f);
        EXPECT_EQ(registry.get<Physics::CharacterBody>(player).halfExtents, World::PlayerHalfExtents);
        EXPECT_TRUE(registry.all_of<Renderer::CameraLens>(player));
    }

    TEST(Player, EyesAreAboveBodyAndLookAlongLookAngles)
    {
        const Core::Transform body{.position = {0.0f, 1.0f, 0.0f}};
        const PlayerLook look{.yaw = 0.3f, .pitch = -0.2f};

        const Core::Transform eyes = CalculatePlayerEyeTransform(body, look);

        EXPECT_NEAR(eyes.position.y, 1.0f + PlayerEyeHeight, Tolerance);
        const glm::quat expected = CalculateCameraRotation(0.3f, -0.2f);
        EXPECT_NEAR(glm::dot(eyes.rotation, expected), 1.0f, Tolerance); // the same rotation
    }

    TEST(MouseLook, MouseRightTurnsRightAndPitchIsClamped)
    {
        float yaw = 0.0f;
        float pitch = 0.0f;

        TurnByMouse(yaw, pitch, {100.0f, -100000.0f}, 0.01f);

        EXPECT_NEAR(yaw, -1.0f, Tolerance);   // right is a negative yaw
        EXPECT_FLOAT_EQ(pitch, MaxLookPitch); // far up, but not beyond the limit
    }

    TEST(PlayerController, TurnsViewOnlyWhileLookingAround)
    {
        Input::InputDevices devices;
        const Input::InputBindings bindings = Input::InputBindings::CreateDefault();
        Input::ActionStates actions;
        const PlayerController controller(PlayerControllerSettings{.mouseSensitivity = 0.01f});
        PlayerLook look;

        // Without the right mouse button the mouse does not turn the view.
        devices.mouse.StartFrame();
        devices.mouse.Move({100.0f, 0.0f});
        actions.Update(devices, bindings);
        controller.UpdateRotation(look, actions, devices.mouse);
        EXPECT_FLOAT_EQ(look.yaw, 0.0f);

        // The frame the button goes down is skipped (the mouse switches to relative mode then); the next one turns.
        devices.mouse.StartFrame();
        devices.mouse.PressButton(Input::MouseButton::Right);
        actions.Update(devices, bindings);
        controller.UpdateRotation(look, actions, devices.mouse);
        devices.mouse.StartFrame();
        devices.mouse.Move({100.0f, 0.0f});
        actions.Update(devices, bindings);
        controller.UpdateRotation(look, actions, devices.mouse);

        EXPECT_NEAR(look.yaw, -1.0f, Tolerance);
    }
}
