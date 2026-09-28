#include "Core/FrameTimer.h"

#include <algorithm>

namespace Abomination::Core
{
    FrameTimer::FrameTimer(TimePoint startTime) noexcept
        : m_previousFrameTime(startTime)
    {}

    void FrameTimer::StartFrame(TimePoint now) noexcept
    {
        // The difference of two time points is a duration in clock ticks; duration<double> converts it to seconds.
        const double elapsedSeconds = std::chrono::duration<double>(now - m_previousFrameTime).count();
        const double deltaTime = std::clamp(elapsedSeconds, 0.0, MaxDeltaTime);

        m_previousFrameTime = now;
        m_deltaTime = static_cast<float>(deltaTime);
    }

    float FrameTimer::GetDeltaTime() const noexcept
    {
        return m_deltaTime;
    }
}
