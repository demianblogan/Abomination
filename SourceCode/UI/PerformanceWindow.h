#pragma once

#include <string>
#include <string_view>

namespace Abomination::Core
{
    class FixedTimestep;
    class FrameLimiter;
    class FrameStatistics;
}

namespace Abomination::Platform
{
    class Window;
}

namespace Abomination::UI
{
    // The Performance window of the debug overlay (View > Performance): a small window pinned to the top-left corner with
    // the version, the GPU, FPS, frame time and its graph, and the simulation ticks per second.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawPerformanceWindow(bool* isOpen, const Core::FrameStatistics& frameStatistics,
                               const Core::FixedTimestep& fixedTimestep, const Platform::Window& window,
                               const Core::FrameLimiter& frameLimiter, std::string_view GPUName);

    // "Unlimited" or "144 FPS": the text of a frame rate limit, in this window and in the Settings menu.
    [[nodiscard]] std::string FormatFramesPerSecondLimit(int maxFramesPerSecond);
}
