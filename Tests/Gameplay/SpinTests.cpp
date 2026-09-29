#include "Core/Scene/Transform.h"
#include "Gameplay/Spin.h"

#include <entt/entt.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr float Tolerance = 1e-5f;

        // The angle between two rotations, in radians: how far one has to turn to get to the other.
        float AngleBetween(glm::quat first, glm::quat second)
        {
            return glm::angle(glm::inverse(first) * second);
        }
    }

    TEST(Spin, TurnsBySpeedTimesDeltaTime)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Core::Transform>(entity);
        registry.emplace<Spin>(entity, Spin{.axis = {0.0f, 1.0f, 0.0f}, .speed = 2.0f});

        UpdateSpinningEntities(registry, 0.25f);

        // 2 rad/s for 0.25 s = 0.5 rad around +Y.
        const glm::quat expected = glm::angleAxis(0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
        EXPECT_NEAR(AngleBetween(registry.get<Core::Transform>(entity).rotation, expected), 0.0f, Tolerance);
    }

    TEST(Spin, EntitiesWithoutSpinDoNotTurn)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Core::Transform>(entity);

        UpdateSpinningEntities(registry, 1.0f);

        EXPECT_NEAR(AngleBetween(registry.get<Core::Transform>(entity).rotation, glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), 0.0f,
                    Tolerance);
    }

    TEST(Spin, ZeroAxisIsIgnored)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Core::Transform>(entity);
        registry.emplace<Spin>(entity, Spin{.axis = {0.0f, 0.0f, 0.0f}, .speed = 5.0f});

        UpdateSpinningEntities(registry, 1.0f);

        const glm::quat rotation = registry.get<Core::Transform>(entity).rotation;
        EXPECT_NEAR(AngleBetween(rotation, glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), 0.0f, Tolerance);
    }
}
