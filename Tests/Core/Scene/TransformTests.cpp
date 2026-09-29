#include "Core/Scene/Transform.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>

namespace Abomination::Core
{
    namespace
    {
        constexpr float Tolerance = 1e-5f;

        // Where the model matrix of the transform puts a point given in the mesh's own coordinates.
        glm::vec3 TransformPoint(const Transform& transform, glm::vec3 point)
        {
            return glm::vec3(CalculateModelMatrix(transform) * glm::vec4(point, 1.0f));
        }

        void ExpectNear(glm::vec3 actual, glm::vec3 expected)
        {
            EXPECT_NEAR(actual.x, expected.x, Tolerance);
            EXPECT_NEAR(actual.y, expected.y, Tolerance);
            EXPECT_NEAR(actual.z, expected.z, Tolerance);
        }
    }

    TEST(Transform, DefaultTransformChangesNothing)
    {
        ExpectNear(TransformPoint(Transform{}, {1.0f, 2.0f, 3.0f}), {1.0f, 2.0f, 3.0f});
    }

    TEST(Transform, ScalesThenRotatesThenMoves)
    {
        // Scale 2, turn 90 degrees around +Y (counter-clockwise seen from above: +X goes to -Z), then move by (10, 0, 0).
        const Transform transform{
            .position = {10.0f, 0.0f, 0.0f},
            .rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
            .scale = glm::vec3(2.0f),
        };

        // (1, 0, 0) -> scaled (2, 0, 0) -> turned (0, 0, -2) -> moved (10, 0, -2).
        ExpectNear(TransformPoint(transform, {1.0f, 0.0f, 0.0f}), {10.0f, 0.0f, -2.0f});
    }
}
