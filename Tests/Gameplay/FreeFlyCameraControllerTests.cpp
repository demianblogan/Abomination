#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/FreeFlyCameraController.h"
#include "Input/ActionStates.h"
#include "Input/InputBindings.h"
#include "Input/InputDevices.h"
#include "Renderer/Camera/CameraLens.h"

#include <entt/entt.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr float Tolerance = 1e-5f;

        using Core::LocalForward;
        using Core::LocalRight;

        void ExpectVectorNear(glm::vec3 actual, glm::vec3 expected)
        {
            EXPECT_NEAR(actual.x, expected.x, Tolerance);
            EXPECT_NEAR(actual.y, expected.y, Tolerance);
            EXPECT_NEAR(actual.z, expected.z, Tolerance);
        }
    }

    // Simulates frames of input with the default bindings and runs the controller on a camera at the origin.
    class FreeFlyCameraControllerTest : public ::testing::Test
    {
    protected:
        // Speed 2 m/s and a fast multiplier of 3 make the expected distances easy to see.
        static constexpr Gameplay::FreeFlyCameraSettings Settings{
            .moveSpeed = 2.0f,
            .fastMoveMultiplier = 3.0f,
            .mouseSensitivity = 0.01f,
        };

        void StartFrame()
        {
            m_devices.keyboard.StartFrame();
            m_devices.mouse.StartFrame();
        }

        // Updates the actions from the devices and runs the controller for one frame with exactly one tick of deltaTime.
        void RunFrame(float deltaTime)
        {
            m_actions.Update(m_devices, m_bindings);
            m_controller.UpdateRotation(m_camera, m_transform, m_actions, m_devices.mouse);
            m_controller.UpdateMovement(m_transform, m_actions, deltaTime);
        }

        // Turns the camera directly, as if the mouse had turned it earlier.
        void SetAngles(float yaw, float pitch)
        {
            m_camera.yaw = yaw;
            m_camera.pitch = pitch;
            m_transform.rotation = CalculateCameraRotation({yaw, pitch});
        }

        Input::InputDevices m_devices;
        Input::InputBindings m_bindings = Input::InputBindings::CreateDefault();
        Input::ActionStates m_actions;
        LookAngles m_camera;
        Core::Transform m_transform;
        FreeFlyCameraController m_controller{Settings};
    };

    TEST_F(FreeFlyCameraControllerTest, MovesForwardWhereCameraLooks)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Input::Key::W);

        RunFrame(0.5f);

        // 2 m/s for 0.5 s along -Z.
        ExpectVectorNear(m_transform.position, {0.0f, 0.0f, -1.0f});
    }

    TEST_F(FreeFlyCameraControllerTest, OppositeActionsCancelOut)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Input::Key::W);
        m_devices.keyboard.PressKey(Input::Key::S);

        RunFrame(1.0f);

        ExpectVectorNear(m_transform.position, {0.0f, 0.0f, 0.0f});
    }

    TEST_F(FreeFlyCameraControllerTest, DiagonalMovementIsNotFaster)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Input::Key::W);
        m_devices.keyboard.PressKey(Input::Key::D);

        RunFrame(1.0f);

        // 2 meters in total, split equally between forward (-Z) and right (+X): 2 / sqrt(2) each.
        const float sideDistance = 2.0f / glm::sqrt(2.0f);
        ExpectVectorNear(m_transform.position, {sideDistance, 0.0f, -sideDistance});
    }

    TEST_F(FreeFlyCameraControllerTest, UpAndDownFollowWorldAxisEvenWhenLookingUp)
    {
        SetAngles(0.0f, glm::radians(60.0f));
        StartFrame();
        m_devices.keyboard.PressKey(Input::Key::E);

        RunFrame(1.0f);

        ExpectVectorNear(m_transform.position, {0.0f, 2.0f, 0.0f});
    }

    TEST_F(FreeFlyCameraControllerTest, MoveFasterMultipliesSpeed)
    {
        StartFrame();
        m_devices.keyboard.PressKey(Input::Key::Q);
        m_devices.keyboard.PressKey(Input::Key::LeftShift);

        RunFrame(1.0f);

        // Down at 2 m/s x 3.
        ExpectVectorNear(m_transform.position, {0.0f, -6.0f, 0.0f});
    }

    TEST_F(FreeFlyCameraControllerTest, MouseTurnsCameraOnlyInLookAroundMode)
    {
        StartFrame();
        m_devices.mouse.Move({100.0f, 0.0f});
        RunFrame(0.016f);

        EXPECT_FLOAT_EQ(m_camera.yaw, 0.0f);
    }

    TEST_F(FreeFlyCameraControllerTest, MouseMovementIsIgnoredInFrameLookAroundModeStarts)
    {
        StartFrame();
        m_devices.mouse.PressButton(Input::MouseButton::Right);
        m_devices.mouse.Move({500.0f, 0.0f});
        RunFrame(0.016f);

        EXPECT_FLOAT_EQ(m_camera.yaw, 0.0f);
    }

    TEST_F(FreeFlyCameraControllerTest, MouseRightTurnsRightAndMouseUpLooksUp)
    {
        StartFrame();
        m_devices.mouse.PressButton(Input::MouseButton::Right);
        RunFrame(0.016f);

        StartFrame();
        m_devices.mouse.Move({10.0f, -20.0f});
        RunFrame(0.016f);

        // 10 pixels right x 0.01 = 0.1 rad to the right (negative yaw); 20 pixels up = 0.2 rad up (positive pitch).
        EXPECT_NEAR(m_camera.yaw, -0.1f, Tolerance);
        EXPECT_NEAR(m_camera.pitch, 0.2f, Tolerance);

        // The transform follows the angles.
        ExpectVectorNear(m_transform.rotation * LocalForward, CalculateCameraRotation({-0.1f, 0.2f}) * LocalForward);
    }

    TEST_F(FreeFlyCameraControllerTest, PitchIsClampedBelowStraightUpAndDown)
    {
        StartFrame();
        m_devices.mouse.PressButton(Input::MouseButton::Right);
        RunFrame(0.016f);

        // 1000 pixels up would be 10 rad of pitch.
        StartFrame();
        m_devices.mouse.Move({0.0f, -1000.0f});
        RunFrame(0.016f);
        EXPECT_FLOAT_EQ(m_camera.pitch, MaxLookPitch);

        StartFrame();
        m_devices.mouse.Move({0.0f, 5000.0f});
        RunFrame(0.016f);
        EXPECT_FLOAT_EQ(m_camera.pitch, -MaxLookPitch);
    }

    TEST(CameraRotation, PositiveYawTurnsLeft)
    {
        const glm::quat rotation = CalculateCameraRotation({glm::radians(90.0f), 0.0f});

        ExpectVectorNear(rotation * LocalForward, {-1.0f, 0.0f, 0.0f});
        ExpectVectorNear(rotation * LocalRight, {0.0f, 0.0f, -1.0f});
    }

    TEST(CameraRotation, PositivePitchLooksUp)
    {
        const glm::vec3 forward = CalculateCameraRotation({0.0f, glm::radians(45.0f)}) * LocalForward;

        EXPECT_GT(forward.y, 0.0f);
        EXPECT_NEAR(glm::length(forward), 1.0f, Tolerance);
    }

    TEST(CameraRotation, RightStaysHorizontalWhenLookingUp)
    {
        const glm::quat rotation = CalculateCameraRotation({glm::radians(30.0f), glm::radians(60.0f)});
        const glm::vec3 right = rotation * LocalRight;

        EXPECT_NEAR(right.y, 0.0f, Tolerance);
        EXPECT_NEAR(glm::dot(right, rotation * LocalForward), 0.0f, Tolerance);
    }

    TEST(FreeFlyCamera, SpawnCreatesCameraEntity)
    {
        entt::registry registry;

        const entt::entity camera = SpawnFreeFlyCamera(registry, {1.0f, 2.0f, 3.0f});

        EXPECT_TRUE((registry.all_of<Core::Transform, Core::PreviousTransform, Renderer::CameraLens, LookAngles>(camera)));
        ExpectVectorNear(registry.get<Core::Transform>(camera).position, {1.0f, 2.0f, 3.0f});
        ExpectVectorNear(registry.get<Core::PreviousTransform>(camera).value.position, {1.0f, 2.0f, 3.0f});
    }
}
