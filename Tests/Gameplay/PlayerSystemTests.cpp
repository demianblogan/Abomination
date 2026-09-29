#include "Gameplay/PlayerSystem.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(PlayerSystem, SlowLandingIsSilent)
    {
        EXPECT_EQ(CalculateLandingVolume(0.0f), 0.0f);
        EXPECT_EQ(CalculateLandingVolume(MinimumLandingSoundSpeed - 0.1f), 0.0f);
    }

    TEST(PlayerSystem, LandingGetsLouderWithSpeedUpToFullVolume)
    {
        const float quietest = CalculateLandingVolume(MinimumLandingSoundSpeed);
        const float afterJump = CalculateLandingVolume(8.4f);

        EXPECT_NEAR(quietest, 1.0f / 3.0f, 1e-6f);
        EXPECT_GT(afterJump, quietest);
        EXPECT_LT(afterJump, 1.0f);
        EXPECT_FLOAT_EQ(CalculateLandingVolume(FullLandingSoundSpeed), 1.0f);
        EXPECT_FLOAT_EQ(CalculateLandingVolume(100.0f), 1.0f);
    }
}
