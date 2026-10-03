#include "Gameplay/Weapons/PumpAction.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(PumpAction, NothingMovesDuringTheRecoil)
    {
        const PumpActionSettings settings;
        const float duringRecoil = settings.recoilDuration * 0.99f;
        EXPECT_FLOAT_EQ(CalculateChestPose(settings, duringRecoil), 0.0f);
        EXPECT_FLOAT_EQ(CalculatePumpProgress(settings, duringRecoil), 0.0f);
    }

    TEST(PumpAction, WeaponReachesTheChestBeforeThePumpMoves)
    {
        const PumpActionSettings settings;
        const float pumpStart = CalculatePumpStartTime(settings);
        EXPECT_FLOAT_EQ(pumpStart, settings.recoilDuration + settings.raiseDuration);

        // While the weapon is raised, the pump stays; when it starts, the weapon is at the chest.
        EXPECT_GT(CalculateChestPose(settings, pumpStart - settings.raiseDuration * 0.5f), 0.0f);
        EXPECT_FLOAT_EQ(CalculatePumpProgress(settings, pumpStart * 0.999f), 0.0f);
        EXPECT_FLOAT_EQ(CalculateChestPose(settings, pumpStart), 1.0f);
    }

    TEST(PumpAction, PumpGoesBackStaysAndComesForwardAtTheChest)
    {
        const PumpActionSettings settings;
        const float backEnd = CalculatePumpStartTime(settings) + settings.backDuration;

        // Halfway back it is already more than halfway (it starts fast and slows down).
        EXPECT_GT(CalculatePumpProgress(settings, CalculatePumpStartTime(settings) + settings.backDuration * 0.5f), 0.5f);
        EXPECT_FLOAT_EQ(CalculatePumpProgress(settings, backEnd + settings.holdDuration * 0.5f), 1.0f);

        // Halfway forward it is halfway (smoothstep is symmetric), and the weapon is still at the chest.
        const float forwardMiddle = backEnd + settings.holdDuration + settings.forwardDuration * 0.5f;
        EXPECT_NEAR(CalculatePumpProgress(settings, forwardMiddle), 0.5f, 1e-4f);
        EXPECT_FLOAT_EQ(CalculateChestPose(settings, forwardMiddle), 1.0f);
    }

    TEST(PumpAction, EverythingIsBackAtTheEndOfTheCycle)
    {
        const PumpActionSettings settings;
        const float cycle = CalculateShotCycleDuration(settings);
        EXPECT_FLOAT_EQ(CalculatePumpProgress(settings, cycle), 0.0f);
        EXPECT_FLOAT_EQ(CalculateChestPose(settings, cycle), 0.0f);

        // The default reload (everything after the recoil) takes 0.65 s.
        EXPECT_NEAR(cycle - settings.recoilDuration, 0.65f, 1e-5f);
    }
}
