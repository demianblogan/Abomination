#include "Core/Time/FrameLimiter.h"

#include <gtest/gtest.h>

#include <chrono>

namespace Abomination::Core
{
    using namespace std::chrono_literals;

    TEST(FrameLimiter, HasNoLimitByDefault)
    {
        const FrameLimiter limiter;
        const TimePoint start{};

        EXPECT_EQ(limiter.GetMaxFPS(), 0);
        EXPECT_EQ(limiter.GetWaitTime(start, start + 1ms), Duration::zero());
    }

    TEST(FrameLimiter, WaitsForRestOfFrame)
    {
        FrameLimiter limiter;
        limiter.SetMaxFPS(50);  // 20 ms per frame
        const TimePoint start{};

        EXPECT_EQ(limiter.GetWaitTime(start, start + 5ms), 15ms);
    }

    TEST(FrameLimiter, DoesNotWaitAfterLongFrame)
    {
        FrameLimiter limiter;
        limiter.SetMaxFPS(50);
        const TimePoint start{};

        EXPECT_EQ(limiter.GetWaitTime(start, start + 20ms), Duration::zero());
        EXPECT_EQ(limiter.GetWaitTime(start, start + 35ms), Duration::zero());
    }

    TEST(FrameLimiter, MinimumFrameDurationIsOneOverLimit)
    {
        FrameLimiter limiter;
        limiter.SetMaxFPS(60);
        const TimePoint start{};

        // A frame that did nothing waits the whole 1/60 s: 16'666'666 ns (the fraction of a nanosecond is dropped).
        EXPECT_EQ(limiter.GetWaitTime(start, start), 16'666'666ns);
    }

    TEST(FrameLimiter, ZeroOrNegativeLimitRemovesLimit)
    {
        FrameLimiter limiter;
        const TimePoint start{};

        limiter.SetMaxFPS(30);
        limiter.SetMaxFPS(0);
        EXPECT_EQ(limiter.GetMaxFPS(), 0);
        EXPECT_EQ(limiter.GetWaitTime(start, start), Duration::zero());

        limiter.SetMaxFPS(-5);
        EXPECT_EQ(limiter.GetMaxFPS(), 0);
        EXPECT_EQ(limiter.GetWaitTime(start, start), Duration::zero());
    }
}
