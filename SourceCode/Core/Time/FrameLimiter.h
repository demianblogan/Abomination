#pragma once

#include "Core/Time/Clock.h"

namespace Abomination::Core
{
    // Keeps the frame rate at or below a chosen limit: calculates how long a finished frame still has to wait
    // so that it lasts at least 1 / limit seconds. Waiting itself is done by the caller (Platform::SleepPrecisely),
    // so the calculation can be tested with exact, made-up time points, like FrameTimer.
    class FrameLimiter
    {
    public:
        // Frames per second at most; 0 or less means no limit.
        void SetMaxFramesPerSecond(int maxFramesPerSecond) noexcept;

        // 0 when there is no limit.
        [[nodiscard]] int GetMaxFramesPerSecond() const noexcept;

        // How long to wait after a frame that started at frameStartTime, if it is now. Zero when there is no limit
        // or the frame already took longer than 1 / limit seconds.
        [[nodiscard]] Duration GetWaitTime(TimePoint frameStartTime, TimePoint now) const noexcept;

    private:
        int m_maxFramesPerSecond = 0;

        // 1 / m_maxFramesPerSecond seconds; calculated once in SetMaxFramesPerSecond(), zero when there is no limit.
        Duration m_minFrameDuration{};
    };
}
