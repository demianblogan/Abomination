#include "Gameplay/Player/LandingDip.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr float FrameDuration = 1.0f / 60.0f;

        // One frame in the air falling at fallSpeed, then one frame on the ground.
        void Land(LandingDip& dip, float fallSpeed)
        {
            UpdateLandingDip(dip, false, -fallSpeed, FrameDuration);
            UpdateLandingDip(dip, true, 0.0f, FrameDuration);
        }
    }

    TEST(LandingDip, LandingDipsTheViewAndItComesBack)
    {
        LandingDip dip;
        Land(dip, 8.0f);
        EXPECT_LT(dip.offset, 0.0f);

        for (int frame = 0; frame < 120; ++frame)
            UpdateLandingDip(dip, true, 0.0f, FrameDuration);
        EXPECT_NEAR(dip.offset, 0.0f, 1e-4f);
    }

    TEST(LandingDip, SlowLandingDoesNotDip)
    {
        LandingDip dip;
        Land(dip, MinimumLandingDipSpeed - 0.5f);

        EXPECT_EQ(dip.offset, 0.0f);
    }

    TEST(LandingDip, HarderLandingDipsDeeper)
    {
        LandingDip soft;
        LandingDip hard;
        Land(soft, 6.0f);
        Land(hard, 12.0f);

        // A few frames later, near the deepest point.
        for (int frame = 0; frame < 5; ++frame)
        {
            UpdateLandingDip(soft, true, 0.0f, FrameDuration);
            UpdateLandingDip(hard, true, 0.0f, FrameDuration);
        }
        EXPECT_LT(hard.offset, soft.offset);
    }
}
