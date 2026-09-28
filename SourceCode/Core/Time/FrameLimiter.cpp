#include "Core/Time/FrameLimiter.h"

#include <chrono>

namespace Abomination::Core
{
    void FrameLimiter::SetMaxFramesPerSecond(int maxFramesPerSecond) noexcept
    {
        if (maxFramesPerSecond <= 0)
        {
            m_maxFramesPerSecond = 0;
            m_minFrameDuration = Duration::zero();

            return;
        }

        m_maxFramesPerSecond = maxFramesPerSecond;

        // duration<double> holds seconds as a double: 1/60 = 0.01666... s. duration_cast then converts it into the ticks
        // of the clock (nanoseconds), dropping the fraction of a nanosecond: 16'666'666 ns.
        const std::chrono::duration<double> minFrameSeconds(1.0 / maxFramesPerSecond);
        m_minFrameDuration = std::chrono::duration_cast<Duration>(minFrameSeconds);
    }

    int FrameLimiter::GetMaxFramesPerSecond() const noexcept
    {
        return m_maxFramesPerSecond;
    }

    Duration FrameLimiter::GetWaitTime(TimePoint frameStartTime, TimePoint now) const noexcept
    {
        if (m_maxFramesPerSecond == 0)
            return Duration::zero();

        // The frame must not end before this moment.
        const TimePoint earliestFrameEnd = frameStartTime + m_minFrameDuration;
        if (now >= earliestFrameEnd)
            return Duration::zero();

        return earliestFrameEnd - now;
    }
}
