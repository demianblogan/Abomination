#include "Core/Scene/Transform.h"
#include "Gameplay/Effects/Fires.h"
#include "Renderer/Fire.h"
#include "Renderer/Light.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(Fires, FlipbookFramesGoFromTheTopLeftOfTheSheet)
    {
        // Frame 0 is the top left quarter-of-a-quarter: the highest texture coordinates.
        const FlipbookFrame first = CalculateFlipbookFrame(0);
        EXPECT_EQ(first.minimum, glm::vec2(0.0f, 0.75f));
        EXPECT_EQ(first.maximum, glm::vec2(0.25f, 1.0f));

        // Frame 6: the third of the second row.
        const FlipbookFrame sixth = CalculateFlipbookFrame(6);
        EXPECT_EQ(sixth.minimum, glm::vec2(0.5f, 0.5f));
        EXPECT_EQ(sixth.maximum, glm::vec2(0.75f, 0.75f));

        // The last is the bottom right.
        EXPECT_EQ(CalculateFlipbookFrame(15).minimum, glm::vec2(0.75f, 0.0f));
    }

    TEST(Fires, FlickerStaysWithinItsRange)
    {
        for (int step = 0; step < 1000; ++step)
        {
            const float flicker = CalculateFlicker(static_cast<float>(step) * 0.037f, 9.0f, 1.3f);
            EXPECT_GE(flicker, -1.0f);
            EXPECT_LE(flicker, 1.0f);
        }
    }

    TEST(Fires, LightWaversAroundItsBase)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Core::Transform>(entity);
        registry.emplace<Renderer::Light>(entity);
        registry.emplace<Renderer::Flicker>(entity, Renderer::Flicker{.baseIntensity = 10.0f,
                                                                      .basePosition = {1.0f, 2.0f, 3.0f},
                                                                      .strength = 0.2f,
                                                                      .wander = 0.05f});

        Effects effects;
        for (int frame = 0; frame < 120; ++frame)
        {
            UpdateFires(registry, effects, 1.0f / 60.0f);
            const float intensity = registry.get<Renderer::Light>(entity).intensity;
            EXPECT_GE(intensity, 8.0f - 1e-4f);
            EXPECT_LE(intensity, 12.0f + 1e-4f);

            // Each axis wanders at most 5 cm, so the light is never farther than about 9 cm away.
            EXPECT_LE(glm::length(registry.get<Core::Transform>(entity).position - glm::vec3(1.0f, 2.0f, 3.0f)), 0.087f);
        }
    }

    TEST(Fires, FlameThrowsSparksAndSmokeAtItsRates)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Core::Transform>(entity);
        registry.emplace<Renderer::Flame>(entity, Renderer::Flame{.sparksPerSecond = 4.0f, .smokePerSecond = 2.0f});

        // One second in quarters: 4 sparks and 2 puffs, nothing lost to fractions.
        Effects effects;
        for (int step = 0; step < 4; ++step)
            UpdateFires(registry, effects, 0.25f);

        EXPECT_EQ(effects.particles.GetCount(), 6u);
    }
}
