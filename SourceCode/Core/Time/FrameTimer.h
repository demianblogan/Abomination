#pragma once

#include "Core/Time/Clock.h"

namespace Abomination::Core
{
    // Measures how much time passes between frames.
    // The current time is passed in from outside instead of being read inside, so the timer can be tested
    // with exact, made-up time points.
    class FrameTimer
    {
    public:
        // A frame longer than this (a breakpoint, a dragged window, a loading hitch) is counted as this long,
        // so the game does not try to catch up on seconds of lost time in one frame.
        static constexpr double MaxDeltaTime = 0.25;

        explicit FrameTimer(TimePoint startTime) noexcept;

        // Starts a new frame at the given time: measures the time passed since the previous frame.
        void StartFrame(TimePoint now) noexcept;

        // Duration of the last frame in seconds, at most MaxDeltaTime. 0 before the first StartFrame().
        [[nodiscard]] float GetDeltaTime() const noexcept;

    private:
        TimePoint m_previousFrameTime;
        float m_deltaTime = 0.0f;
    };
}
