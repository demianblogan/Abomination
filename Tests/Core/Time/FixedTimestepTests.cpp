#include "Core/Time/FixedTimestep.h"

#include <gtest/gtest.h>

namespace Abomination::Core
{
    // 4 ticks per second: a tick lasts 0.25 s. Frame times here are sums of powers of two (0.5, 0.25, 0.125),
    // which are exact in float, so the ticks happen exactly on the expected frames.
    class FixedTimestepTest : public ::testing::Test
    {
    protected:
        FixedTimestep m_timestep{4};
    };

    TEST_F(FixedTimestepTest, TickDurationIsOneOverTickRate)
    {
        EXPECT_EQ(m_timestep.GetTicksPerSecond(), 4);
        EXPECT_FLOAT_EQ(m_timestep.GetTickDuration(), 0.25f);
    }

    TEST_F(FixedTimestepTest, ShortFramesAccumulateUntilTickHasPassed)
    {
        // A frame shorter than a tick: no tick yet, half of the next tick has passed.
        EXPECT_EQ(m_timestep.Advance(0.125f), 0);
        EXPECT_FLOAT_EQ(m_timestep.GetInterpolationFactor(), 0.5f);

        // The second half: exactly one tick, nothing left.
        EXPECT_EQ(m_timestep.Advance(0.125f), 1);
        EXPECT_FLOAT_EQ(m_timestep.GetInterpolationFactor(), 0.0f);
    }

    TEST_F(FixedTimestepTest, LongFrameRunsSeveralTicks)
    {
        EXPECT_EQ(m_timestep.Advance(0.75f), 3);
    }

    TEST_F(FixedTimestepTest, RestIsCarriedToNextFrame)
    {
        // 0.375 s = 1 tick + 0.125 s left (half a tick).
        EXPECT_EQ(m_timestep.Advance(0.375f), 1);
        EXPECT_FLOAT_EQ(m_timestep.GetInterpolationFactor(), 0.5f);

        // 0.125 s left + 0.375 s = 0.5 s = 2 ticks: the rest of the previous frame was not lost.
        EXPECT_EQ(m_timestep.Advance(0.375f), 2);
        EXPECT_FLOAT_EQ(m_timestep.GetInterpolationFactor(), 0.0f);
    }

    TEST_F(FixedTimestepTest, TickCountIsLimitedAndExtraTimeIsDropped)
    {
        // 10 s would be 40 ticks; only MaxTicksPerFrame are run and the rest of the time is dropped,
        // so the next short frame does not have to catch up on it.
        EXPECT_EQ(m_timestep.Advance(10.0f), FixedTimestep::MaxTicksPerFrame);
        EXPECT_EQ(m_timestep.Advance(0.125f), 0);
    }

    TEST_F(FixedTimestepTest, NegativeFrameTimeIsIgnored)
    {
        EXPECT_EQ(m_timestep.Advance(-1.0f), 0);
        EXPECT_FLOAT_EQ(m_timestep.GetInterpolationFactor(), 0.0f);
    }

    TEST(FixedTimestep, SixtyTicksPerSecondRegardlessOfFrameRate)
    {
        // One second of frames at different frame rates always gives 60 ticks (±1 for float rounding at the end
        // of the second: the 60th tick may still be in the accumulator).
        for (const int framesPerSecond : {15, 30, 60, 144, 1000})
        {
            FixedTimestep timestep(60);
            int tickCount = 0;
            for (int frame = 0; frame < framesPerSecond; ++frame)
                tickCount += timestep.Advance(1.0f / static_cast<float>(framesPerSecond));

            EXPECT_NEAR(tickCount, 60, 1) << "at " << framesPerSecond << " FPS";
        }
    }
}
