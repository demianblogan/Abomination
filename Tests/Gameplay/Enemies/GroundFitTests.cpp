#include "Gameplay/Enemies/GroundFit.h"

#include <glm/trigonometric.hpp>
#include <gtest/gtest.h>

#include <cmath>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr float PawDistance = 0.4f;
        constexpr float MaximumTilt = glm::radians(30.0f);
        constexpr float MaximumOffset = 0.56f;
    }

    TEST(GroundFit, FlatGroundKeepsTheModelOnTheBox)
    {
        const GroundFitTarget target = CalculateGroundFitTarget(1.0f, 1.0f, 1.0f, PawDistance, MaximumTilt, MaximumOffset);

        EXPECT_FLOAT_EQ(target.offset, 0.0f);
        EXPECT_FLOAT_EQ(target.pitch, 0.0f);
    }

    TEST(GroundFit, FrontPawsOverALowerStepLowerAndTiltTheNoseDown)
    {
        // The box rests on the upper step (bottom 0.5); the front paws are over the floor a step (0.25 m) below.
        const GroundFitTarget target = CalculateGroundFitTarget(0.25f, 0.5f, 0.5f, PawDistance, MaximumTilt, MaximumOffset);

        EXPECT_FLOAT_EQ(target.offset, -0.125f);
        EXPECT_NEAR(target.pitch, std::atan2(-0.25f, 0.8f), 1e-6f);
        EXPECT_LT(target.pitch, 0.0f);
    }

    TEST(GroundFit, TiltAndOffsetAreLimited)
    {
        const GroundFitTarget target = CalculateGroundFitTarget(3.0f, 0.0f, 0.0f, PawDistance, MaximumTilt, MaximumOffset);

        EXPECT_FLOAT_EQ(target.pitch, MaximumTilt);
        EXPECT_FLOAT_EQ(target.offset, MaximumOffset);
    }

    TEST(GroundFit, GlidesToTheTargetNoFasterThanItsSpeeds)
    {
        GroundFit fit;
        const GroundFitTarget target{.offset = -0.2f, .pitch = 0.3f};

        UpdateGroundFit(fit, target, 0.0f, 1.0f, 2.0f, 1.0f, 0.1f);
        EXPECT_FLOAT_EQ(fit.offset, -0.1f);
        EXPECT_FLOAT_EQ(fit.pitch, 0.2f);
        EXPECT_FLOAT_EQ(fit.previousOffset, 0.0f);

        UpdateGroundFit(fit, target, 0.0f, 1.0f, 2.0f, 1.0f, 0.1f);
        EXPECT_FLOAT_EQ(fit.offset, -0.2f);
        EXPECT_FLOAT_EQ(fit.pitch, 0.3f);
        EXPECT_FLOAT_EQ(fit.previousOffset, -0.1f);
    }

    TEST(GroundFit, ModelStaysBelowWhenTheBoxStepsUp)
    {
        GroundFit fit;

        // The box jumped up a 0.25 m step: the model starts the glide a step below the box and rises 0.05 m this tick.
        UpdateGroundFit(fit, GroundFitTarget{}, 0.25f, 0.5f, 1.0f, 1.0f, 0.1f);

        EXPECT_FLOAT_EQ(fit.offset, -0.2f);
        EXPECT_FLOAT_EQ(fit.previousOffset, 0.0f);
    }

    TEST(GroundFit, ModelNeverLagsFartherThanTheLimit)
    {
        GroundFit fit;

        // Two steps up in two ticks, faster than the glide: the model stays no more than 0.3 m below the box.
        UpdateGroundFit(fit, GroundFitTarget{}, 0.25f, 0.5f, 1.0f, 0.3f, 0.01f);
        UpdateGroundFit(fit, GroundFitTarget{}, 0.25f, 0.5f, 1.0f, 0.3f, 0.01f);

        EXPECT_FLOAT_EQ(fit.offset, -0.3f);
    }
}
