#include "Gameplay/Effects/Effects.h"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(Effects, WallImpactThrowsParticlesAndLeavesMark)
    {
        Effects effects;
        const glm::vec3 point{1.0f, 2.0f, 3.0f};
        const glm::vec3 normal{0.0f, 0.0f, 1.0f};

        SpawnWallImpact(effects, point, normal);

        const auto expectedParticles = static_cast<std::size_t>(effects.settings.sparkCount + effects.settings.dustCount);
        EXPECT_EQ(effects.particles.GetCount(), expectedParticles);
        ASSERT_EQ(effects.decals.size(), 1u);

        // The mark lies a hair in front of the wall, not inside it.
        EXPECT_GT(glm::dot(effects.decals[0].position - point, normal), 0.0f);
    }

    TEST(Effects, OldestMarksDisappearBeyondTheLimit)
    {
        Effects effects;
        effects.settings.maximumMarkCount = 4;

        for (int shot = 0; shot < 6; ++shot)
            SpawnWallImpact(effects, glm::vec3(static_cast<float>(shot), 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        ASSERT_EQ(effects.decals.size(), 4u);

        // Marks 0 and 1 are gone; the oldest one left is mark 2.
        EXPECT_NEAR(effects.decals.front().position.x, 2.0f, 1e-5f);
    }

    TEST(Effects, ClearRemovesEverything)
    {
        Effects effects;
        SpawnWallImpact(effects, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        SpawnBloodImpact(effects, glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        ClearEffects(effects);

        EXPECT_EQ(effects.particles.GetCount(), 0u);
        EXPECT_TRUE(effects.decals.empty());
    }
}
