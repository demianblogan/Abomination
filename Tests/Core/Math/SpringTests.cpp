#include "Core/Math/Spring.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace Abomination::Core
{
    TEST(Spring, KickedSpringSwingsOutAndSettles)
    {
        float offset = 0.0f;
        float velocity = 1.0f;
        float largest = 0.0f;

        for (int frame = 0; frame < 120; ++frame)
        {
            UpdateDampedSpring(offset, velocity, 150.0f, 1.0f / 60.0f);
            largest = std::max(largest, offset);
        }

        // At most about 0.46 * v / sqrt(k) = 0.46 / 12.2 = 3.8 cm away, then back at 0.
        EXPECT_NEAR(largest, 0.46f / std::sqrt(150.0f), 0.005f);
        EXPECT_NEAR(offset, 0.0f, 1e-4f);
    }

    TEST(Spring, SameMotionAtAnyFrameRate)
    {
        const auto run = [](float deltaTime)
        {
            float offset = 0.0f;
            float velocity = 1.0f;
            for (int frame = 0; frame < static_cast<int>(std::lround(0.1f / deltaTime)); ++frame)
                UpdateDampedSpring(offset, velocity, 150.0f, deltaTime);
            return offset;
        };

        EXPECT_NEAR(run(1.0f / 30.0f), run(1.0f / 240.0f), 1e-4f);
    }

    TEST(Spring, ApproachIsTheSameInOneOrTwoSteps)
    {
        // Two frames of 0.01 s close as much of the gap as one frame of 0.02 s.
        const float oneStep = CalculateApproachFactor(10.0f, 0.02f);
        const float firstStep = CalculateApproachFactor(10.0f, 0.01f);
        const float twoSteps = firstStep + (1.0f - firstStep) * CalculateApproachFactor(10.0f, 0.01f);

        EXPECT_NEAR(oneStep, twoSteps, 1e-6f);
    }
}
