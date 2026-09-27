#include "Core/TransformInterpolation.h"

#include <entt/entt.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

namespace Abomination::Core
{
    namespace
    {
        constexpr float Tolerance = 1e-5f;

        void ExpectNear(glm::vec3 actual, glm::vec3 expected)
        {
            EXPECT_NEAR(actual.x, expected.x, Tolerance);
            EXPECT_NEAR(actual.y, expected.y, Tolerance);
            EXPECT_NEAR(actual.z, expected.z, Tolerance);
        }

        // The angle between two rotations, in radians.
        float AngleBetween(glm::quat first, glm::quat second)
        {
            return glm::angle(glm::inverse(first) * second);
        }

        // From the origin, not turned, size 1 to (10, 0, 0), turned by 90 degrees around +Y, size 3.
        const Transform Previous{};
        const Transform Current{
            .position = {10.0f, 0.0f, 0.0f},
            .rotation = glm::angleAxis(glm::radians(90.0f), WorldUp),
            .scale = glm::vec3(3.0f),
        };
    }

    TEST(TransformInterpolation, EndsGivePreviousAndCurrent)
    {
        const Transform atStart = InterpolateTransform(Previous, Current, 0.0f);
        const Transform atEnd = InterpolateTransform(Previous, Current, 1.0f);

        ExpectNear(atStart.position, Previous.position);
        EXPECT_NEAR(AngleBetween(atStart.rotation, Previous.rotation), 0.0f, Tolerance);
        ExpectNear(atEnd.position, Current.position);
        EXPECT_NEAR(AngleBetween(atEnd.rotation, Current.rotation), 0.0f, Tolerance);
        ExpectNear(atEnd.scale, Current.scale);
    }

    TEST(TransformInterpolation, HalfwayIsHalfOfEverything)
    {
        const Transform halfway = InterpolateTransform(Previous, Current, 0.5f);

        ExpectNear(halfway.position, {5.0f, 0.0f, 0.0f});
        ExpectNear(halfway.scale, glm::vec3(2.0f));

        // slerp turns at a constant speed: halfway between 0 and 90 degrees is 45 degrees around the same axis.
        EXPECT_NEAR(AngleBetween(halfway.rotation, glm::angleAxis(glm::radians(45.0f), WorldUp)), 0.0f, Tolerance);
    }

    TEST(TransformInterpolation, EnableInterpolationStartsFromCurrentTransform)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Transform>(entity, Current);

        EnableInterpolation(registry, entity);

        ExpectNear(registry.get<PreviousTransform>(entity).value.position, Current.position);
    }

    TEST(TransformInterpolation, StorePreviousTransformsCopiesOnlyInterpolatedEntities)
    {
        entt::registry registry;
        const entt::entity interpolated = registry.create();
        registry.emplace<Transform>(interpolated);
        EnableInterpolation(registry, interpolated);
        const entt::entity notInterpolated = registry.create();
        registry.emplace<Transform>(notInterpolated);

        // A tick moves the entity, then the next tick starts by storing where it is now.
        registry.get<Transform>(interpolated).position = {1.0f, 2.0f, 3.0f};
        StorePreviousTransforms(registry);

        ExpectNear(registry.get<PreviousTransform>(interpolated).value.position, {1.0f, 2.0f, 3.0f});
        EXPECT_FALSE(registry.all_of<PreviousTransform>(notInterpolated));
    }
}
