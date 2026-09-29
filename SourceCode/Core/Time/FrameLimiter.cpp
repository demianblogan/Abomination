#include "Core/Time/FrameLimiter.h"

#include <chrono>

namespace Abomination::Core
{
    void FrameLimiter::SetMaxFPS(int maxFPS) noexcept
    {
        if (maxFPS <= 0)
        {
            m_maxFPS = 0;
            m_minFrameDuration = Duration::zero();

            return;
        }

        m_maxFPS = maxFPS;

        // duration<double> holds seconds as a double: 1/60 = 0.01666... s. duration_cast then converts it into the ticks
        // of the clock (nanoseconds), dropping the fraction of a nanosecond: 16'666'666 ns.
        const std::chrono::duration<double> minFrameSeconds(1.0 / maxFPS);
        m_minFrameDuration = std::chrono::duration_cast<Duration>(minFrameSeconds);
    }

    int FrameLimiter::GetMaxFPS() const noexcept
    {
        return m_maxFPS;
    }

    Duration FrameLimiter::GetWaitTime(TimePoint frameStartTime, TimePoint now) const noexcept
    {
        if (m_maxFPS == 0)
            return Duration::zero();

        // The frame must not end before this moment.
        const TimePoint earliestFrameEnd = frameStartTime + m_minFrameDuration;
        if (now >= earliestFrameEnd)
            return Duration::zero();

        return earliestFrameEnd - now;
    }
}
