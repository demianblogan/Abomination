#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace Abomination::Core
{
    // Collects frame durations and simulation tick counts for the performance window. Two kinds of data,
    // for two different uses:
    //
    // 1. Numbers to read (average frame time, FPS, the longest frame, ticks per second) are calculated over intervals
    //    of ReportInterval seconds and change only when an interval ends. Numbers that changed every frame would be
    //    unreadable at high frame rates: at 1700 FPS the last digits flicker hundreds of times per second.
    //
    // 2. The durations of the last MaxSampleCount frames, for the frame time graph, where every frame matters. They are
    //    stored in a ring buffer: a fixed array where every new frame overwrites the oldest one. Once the array is full,
    //    the next sample goes to index 0 again, then 1, and so on, so no memory is ever moved.
    //
    //   after 5 frames (capacity 4):   [ f5 | f2 | f3 | f4 ]    next write -> index 1, oldest sample -> index 1
    class FrameStatistics
    {
    public:
        // How many of the last frames the graph shows. About 2 seconds at 60 FPS.
        static constexpr std::size_t MaxSampleCount = 120;

        // How often, in seconds, the numbers to read are updated: twice per second.
        static constexpr float ReportInterval = 0.5f;

        // Adds a finished frame: its duration in seconds and how many simulation ticks ran in it.
        void AddFrame(float frameTime, int tickCount) noexcept;

        // Average frame duration in seconds over the last finished interval. Until the first interval ends, over the
        // frames added so far, so the window does not show zeros during the first half second. 0 while no frame was added.
        [[nodiscard]] float GetAverageFrameTime() const noexcept;

        // Frames per second derived from GetAverageFrameTime(). 0 while no frame was added.
        [[nodiscard]] float GetAverageFramesPerSecond() const noexcept;

        // Duration of the longest frame in seconds over the same frames as GetAverageFrameTime(). 0 while no frame was added.
        [[nodiscard]] float GetLongestFrameTime() const noexcept;

        // Simulation ticks per second actually run, over the same frames as GetAverageFrameTime(). Equals the tick rate
        // of the fixed timestep at any frame rate, unless the computer is too slow and ticks are dropped.
        [[nodiscard]] float GetTicksPerSecond() const noexcept;

        // The remembered frame times in storage order (not chronological once the buffer has wrapped around).
        // Together with GetOldestSampleIndex() this is exactly what ImGui::PlotLines expects for a ring buffer.
        [[nodiscard]] std::span<const float> GetFrameTimeSamples() const noexcept;

        // Index in GetFrameTimeSamples() of the oldest remembered frame: the graph starts there.
        [[nodiscard]] std::size_t GetOldestSampleIndex() const noexcept;

    private:
        // --- The graph ---
        std::array<float, MaxSampleCount> m_frameTimeSamples{};

        // Where the next sample will be written.
        std::size_t m_nextIndex = 0;

        // How many samples are stored: grows up to MaxSampleCount and then stays there.
        std::size_t m_storedSampleCount = 0;

        // --- The current interval: collects frames until ReportInterval seconds have passed ---
        float m_intervalTime = 0.0f;
        std::size_t m_intervalFrameCount = 0;
        int m_intervalTickCount = 0;
        float m_intervalLongestFrameTime = 0.0f;

        // --- The result of the last finished interval, returned by the getters ---
        bool m_wasFirstIntervalFinished = false;
        float m_reportedAverageFrameTime = 0.0f;
        float m_reportedLongestFrameTime = 0.0f;
        float m_reportedTicksPerSecond = 0.0f;
    };
}
