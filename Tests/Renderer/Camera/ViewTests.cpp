#include "Renderer/Camera/View.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>

namespace Abomination::Renderer
{
    namespace
    {
        // Results of sin/cos and matrix products differ from exact values in the last digits.
        constexpr float Tolerance = 1e-5f;

        // A camera somewhere in the world, turned 40 degrees to the left and tilted 20 degrees down.
        const Core::Transform TurnedCamera{
            .position = {3.0f, 1.0f, 5.0f},
            .rotation = glm::angleAxis(glm::radians(40.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
                        glm::angleAxis(glm::radians(-20.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        };

        // Where a world point ends up in view space: the camera at the origin looking along -Z.
        glm::vec3 ToViewSpace(const View& view, glm::vec3 worldPoint)
        {
            return glm::vec3(view.viewMatrix * glm::vec4(worldPoint, 1.0f));
        }

        void ExpectVectorNear(glm::vec3 actual, glm::vec3 expected)
        {
            EXPECT_NEAR(actual.x, expected.x, Tolerance);
            EXPECT_NEAR(actual.y, expected.y, Tolerance);
            EXPECT_NEAR(actual.z, expected.z, Tolerance);
        }
    }

    TEST(View, CameraThatIsNotTurnedLooksAlongNegativeZ)
    {
        const View view = CalculateView(Core::Transform{}, CameraLens{}, 1.0f);

        // A point 5 meters along -Z is straight ahead: it stays where it is in view space.
        ExpectVectorNear(ToViewSpace(view, {0.0f, 0.0f, -5.0f}), {0.0f, 0.0f, -5.0f});
    }

    TEST(View, CameraPositionEndsUpAtOrigin)
    {
        const View view = CalculateView(TurnedCamera, CameraLens{}, 1.0f);

        ExpectVectorNear(ToViewSpace(view, TurnedCamera.position), {0.0f, 0.0f, 0.0f});
        ExpectVectorNear(view.position, TurnedCamera.position);
    }

    TEST(View, PointInFrontOfTurnedCameraEndsUpOnNegativeZ)
    {
        const View view = CalculateView(TurnedCamera, CameraLens{}, 1.0f);

        // The camera looks along its rotated -Z; a point 10 meters that way must be straight ahead in view space.
        const glm::vec3 forward = TurnedCamera.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
        const glm::vec3 pointInFront = TurnedCamera.position + forward * 10.0f;

        ExpectVectorNear(ToViewSpace(view, pointInFront), {0.0f, 0.0f, -10.0f});
    }

    TEST(View, WiderWindowSqueezesXInProjection)
    {
        // In a window twice as wide as high, the same point gets half the horizontal clip-space coordinate:
        // the projection makes room for the extra width, so a square stays a square on the screen.
        const View square = CalculateView(Core::Transform{}, CameraLens{}, 1.0f);
        const View wide = CalculateView(Core::Transform{}, CameraLens{}, 2.0f);
        const glm::vec4 point(1.0f, 0.0f, -5.0f, 1.0f);

        EXPECT_NEAR((wide.projectionMatrix * point).x, (square.projectionMatrix * point).x / 2.0f, Tolerance);
    }
}
