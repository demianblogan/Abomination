#include "Core/FixedTimestep.h"

#include <algorithm>
#include <cassert>

namespace Abomination::Core
{
    FixedTimestep::FixedTimestep(int ticksPerSecond) noexcept
        : m_ticksPerSecond(ticksPerSecond)
        , m_tickDuration(1.0f / static_cast<float>(ticksPerSecond))
    {
        // 0 ticks per second would make the tick infinitely long. An assertion checks a mistake of the programmer, not a
        // situation the game handles: in Debug builds it stops the program right here (with the debugger attached, on
        // this line); in Release builds it is removed and costs nothing.
        assert(ticksPerSecond > 0);
    }

    int FixedTimestep::Advance(float frameTime) noexcept
    {
        m_accumulator += std::max(frameTime, 0.0f);

        // Take whole ticks out of the accumulator while they fit; only the part of a tick that has not passed yet is left.
        // Subtracting one tick at a time (instead of dividing) keeps the rest exactly in [0, tick duration), which
        // keeps the interpolation factor in [0, 1). Even a frame of 0.25 s (FrameTimer::MaxDeltaTime) is only 15 steps.
        int availableTickCount = 0;
        while (m_accumulator >= m_tickDuration)
        {
            m_accumulator -= m_tickDuration;
            ++availableTickCount;
        }

        // Ticks above the limit are dropped: that time is never simulated, and the game falls behind the real time.
        return std::min(availableTickCount, MaxTicksPerFrame);
    }

    float FixedTimestep::GetTickDuration() const noexcept
    {
        return m_tickDuration;
    }

    int FixedTimestep::GetTicksPerSecond() const noexcept
    {
        return m_ticksPerSecond;
    }

    float FixedTimestep::GetInterpolationFactor() const noexcept
    {
        return m_accumulator / m_tickDuration;
    }
}
