#pragma once

#include <chrono>

// Short names for the clock of the game and its two kinds of values, so code reads TimePoint instead of
// std::chrono::steady_clock::time_point.
namespace Abomination::Core
{
    // A monotonic clock: it never jumps back, even if the user changes the system time. Counts in nanoseconds.
    using Clock = std::chrono::steady_clock;

    // A moment in time: "when". Only the difference of two time points means something.
    using TimePoint = Clock::time_point;

    // A length of time: "how long". The difference of two time points; TimePoint + Duration is a TimePoint again.
    using Duration = Clock::duration;
}
