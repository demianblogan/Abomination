#include "Core/Time/FrameStatistics.h"

#include <gtest/gtest.h>

#include <cstddef>

namespace Abomination::Core
{
    // Averages are sums of floats, so they may differ from the exact value in the last digits.
    constexpr float Tolerance = 1e-6f;

    TEST(FrameStatistics, EverythingIsZeroWithoutFrames)
    {
        const FrameStatistics statistics;

        EXPECT_FLOAT_EQ(statistics.GetAverageFrameTime(), 0.0f);
        EXPECT_FLOAT_EQ(statistics.GetAverageFramesPerSecond(), 0.0f);
        EXPECT_FLOAT_EQ(statistics.GetLongestFrameTime(), 0.0f);
        EXPECT_TRUE(statistics.GetFrameTimeSamples().empty());
    }

    TEST(FrameStatistics, AveragesStoredFrames)
    {
        FrameStatistics statistics;

        statistics.AddFrame(0.010f, 0);
        statistics.AddFrame(0.020f, 0);
        statistics.AddFrame(0.030f, 0);

        EXPECT_NEAR(statistics.GetAverageFrameTime(), 0.020f, Tolerance);
        EXPECT_NEAR(statistics.GetAverageFramesPerSecond(), 50.0f, 0.001f);
    }

    // The following tests use powers of two (0.25, 0.125, 0.0625): they are exact in float, so the interval ends
    // exactly on the expected frame. ReportInterval is 0.5 s.
    TEST(FrameStatistics, NumbersChangeOnlyWhenIntervalEnds)
    {
        FrameStatistics statistics;

        // 4 x 0.125 s = 0.5 s: the first interval ends with an average of 0.125 s (8 FPS).
        for (int frame = 0; frame < 4; ++frame)
            statistics.AddFrame(0.125f, 0);
        EXPECT_FLOAT_EQ(statistics.GetAverageFrameTime(), 0.125f);

        // Faster frames do not change the numbers until the next interval ends...
        for (int frame = 0; frame < 7; ++frame)
            statistics.AddFrame(0.0625f, 0);
        EXPECT_FLOAT_EQ(statistics.GetAverageFrameTime(), 0.125f);
        EXPECT_FLOAT_EQ(statistics.GetAverageFramesPerSecond(), 8.0f);

        // ...which happens on the 8th frame: 8 x 0.0625 s = 0.5 s.
        statistics.AddFrame(0.0625f, 0);
        EXPECT_FLOAT_EQ(statistics.GetAverageFrameTime(), 0.0625f);
        EXPECT_FLOAT_EQ(statistics.GetAverageFramesPerSecond(), 16.0f);
    }

    TEST(FrameStatistics, LongestFrameIsForgottenInNextInterval)
    {
        FrameStatistics statistics;

        // First interval: 0.25 + 0.125 + 0.125 = 0.5 s, the longest frame is 0.25 s.
        statistics.AddFrame(0.25f, 0);
        statistics.AddFrame(0.125f, 0);
        statistics.AddFrame(0.125f, 0);
        EXPECT_FLOAT_EQ(statistics.GetLongestFrameTime(), 0.25f);

        // Second interval: only 0.125 s frames, so the long frame is no longer reported.
        for (int frame = 0; frame < 4; ++frame)
            statistics.AddFrame(0.125f, 0);
        EXPECT_FLOAT_EQ(statistics.GetLongestFrameTime(), 0.125f);
    }

    TEST(FrameStatistics, CountsTicksPerSecond)
    {
        FrameStatistics statistics;

        // One interval of 0.5 s: 4 frames with 2, 0, 1 and 1 ticks = 4 ticks in 0.5 s = 8 ticks per second.
        statistics.AddFrame(0.125f, 2);
        statistics.AddFrame(0.125f, 0);
        statistics.AddFrame(0.125f, 1);
        statistics.AddFrame(0.125f, 1);

        EXPECT_FLOAT_EQ(statistics.GetTicksPerSecond(), 8.0f);
    }

    TEST(FrameStatistics, FindsLongestFrame)
    {
        FrameStatistics statistics;

        statistics.AddFrame(0.016f, 0);
        statistics.AddFrame(0.080f, 0);
        statistics.AddFrame(0.017f, 0);

        EXPECT_FLOAT_EQ(statistics.GetLongestFrameTime(), 0.080f);
    }

    TEST(FrameStatistics, OldestSampleIsFirstUntilBufferIsFull)
    {
        FrameStatistics statistics;

        statistics.AddFrame(0.016f, 0);
        statistics.AddFrame(0.017f, 0);

        EXPECT_EQ(statistics.GetFrameTimeSamples().size(), 2u);
        EXPECT_EQ(statistics.GetOldestSampleIndex(), 0u);
    }

    TEST(FrameStatistics, NewFramesOverwriteOldestWhenBufferIsFull)
    {
        FrameStatistics statistics;

        // One very long frame first, then enough short frames to fill the buffer and push the long one out.
        statistics.AddFrame(1.0f, 0);
        for (std::size_t frame = 0; frame < FrameStatistics::MaxSampleCount + 1; ++frame)
            statistics.AddFrame(0.010f, 0);

        EXPECT_EQ(statistics.GetFrameTimeSamples().size(), FrameStatistics::MaxSampleCount);
        EXPECT_EQ(statistics.GetOldestSampleIndex(), 2u);
        EXPECT_FLOAT_EQ(statistics.GetLongestFrameTime(), 0.010f);
        EXPECT_NEAR(statistics.GetAverageFrameTime(), 0.010f, Tolerance);
    }
}
