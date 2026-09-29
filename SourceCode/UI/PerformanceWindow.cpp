#include "UI/PerformanceWindow.h"

#include "Core/BuildConfiguration.h"
#include "Core/Time/FixedTimestep.h"
#include "Core/Time/FrameLimiter.h"
#include "Core/Time/FrameStatistics.h"
#include "Core/Version.h"
#include "Platform/Window.h"
#include "UI/UIScale.h"

#include <imgui.h>

#include <algorithm>
#include <format>
#include <span>

namespace Abomination::UI
{
    namespace
    {
        // Distance from the edges of the game window (below the menu bar) to the window, in pixels.
        constexpr float WindowMargin = 10.0f;

        // Opacity of the window background: 0 is fully transparent, 1 is opaque.
        constexpr float BackgroundAlpha = 0.6f;

        // Size of the frame time graph in pixels.
        constexpr float GraphWidth = 400.0f;
        constexpr float GraphHeight = 60.0f;

        // The top of the graph is at least 1/30 s (33.3 ms): the budget of a frame at 30 FPS. A fixed minimum keeps
        // the scale stable, so the same frame time always has the same height; only longer frames stretch it.
        constexpr float MinimumGraphTopFrameTime = 1.0f / 30.0f;

        constexpr float MillisecondsPerSecond = 1000.0f;

        // The window has a title bar with a close button and can be collapsed by the arrow in it, but it cannot be moved:
        //   AlwaysAutoResize   - the size always fits the contents (so it cannot be resized by hand either);
        //   NoMove             - stays pinned to the corner;
        //   NoSavedSettings    - its position and state are not written anywhere;
        //   NoFocusOnAppearing - does not take the keyboard focus from the game when it appears;
        //   NoNav              - is skipped by keyboard and gamepad navigation between ImGui windows.
        constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                                 ImGuiWindowFlags_NoNav;
    }

    void DrawPerformanceWindow(bool* isOpen, const Core::FrameStatistics& frameStatistics,
                               const Core::FixedTimestep& fixedTimestep, const Platform::Window& window,
                               const Core::FrameLimiter& frameLimiter, std::string_view GPUName)
    {
        // The window is placed below the menu bar, whose height is the height of one line of ImGui widgets.
        // Both calls only affect the next Begin(). ImGuiCond_Always applies the position every frame,
        // so the window stays pinned to the corner.
        const float margin = ScaleToUI(WindowMargin);
        const ImVec2 position(margin, ImGui::GetFrameHeight() + margin);
        ImGui::SetNextWindowPos(position, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(BackgroundAlpha);

        // The title is shown in the title bar; ImGui also identifies windows by it, so it must be unique.
        // Passing the bool adds a close button to the title bar, which sets it to false.
        // Begin() returns false when the window is collapsed or fully clipped: then its contents are skipped,
        // but End() must still be called.
        if (ImGui::Begin("Performance", isOpen, WindowFlags))
        {
            const std::string_view buildConfiguration = Core::IsDebugBuild ? "Debug" : "Release";
            const std::string versionText =
                std::format("Abomination {} ({})", Core::GetGameVersionString(), buildConfiguration);
            ImGui::TextUnformatted(versionText.c_str());
            ImGui::TextUnformatted(GPUName.data(), GPUName.data() + GPUName.size());

            ImGui::Separator();

            const float averageFrameTime = frameStatistics.GetAverageFrameTime();
            const float longestFrameTime = frameStatistics.GetLongestFrameTime();

            // "{:.0f}" - no digits after the point, "{:.2f}" - two digits.
            const std::string FPSText = std::format("FPS: {:.0f}", frameStatistics.GetAverageFPS());
            const std::string frameTimeText =
                std::format("Frame time: {:.2f} ms (longest {:.2f} ms)", averageFrameTime * MillisecondsPerSecond,
                            longestFrameTime * MillisecondsPerSecond);
            const std::string frameRateSettingsText =
                std::format("V-Sync: {}, FPS limit: {}", window.IsVSyncEnabled() ? "on" : "off",
                            FormatFPSLimit(frameLimiter.GetMaxFPS()));
            ImGui::TextUnformatted(FPSText.c_str());
            ImGui::TextUnformatted(frameTimeText.c_str());
            ImGui::TextUnformatted(frameRateSettingsText.c_str());

            // Simulation ticks per second: actually run / target. Both are equal at any FPS; fewer actual ticks mean the
            // computer cannot simulate in real time and Core::FixedTimestep drops ticks, so the game runs slower.
            const std::string simulationText =
                std::format("Simulation: {:.0f} / {} ticks per second", frameStatistics.GetTicksPerSecond(),
                            fixedTimestep.GetTicksPerSecond());
            ImGui::TextUnformatted(simulationText.c_str());
            ImGui::SetItemTooltip("Actual / target. Fewer actual ticks mean the simulation cannot keep up and the game "
                                  "runs slower than real time.");

            // The graph: one point per frame, the height is the frame time. The samples are a ring buffer, so the
            // index of the oldest sample is passed as the offset: ImGui starts drawing from it and wraps around.
            // The "##" prefix hides the label: the text after it is used only as an ID.
            // The top of the graph fits the longest frame on the graph itself, which can be older than the interval
            // of the numbers above (the graph covers MaxSampleCount frames, the numbers only the last half second).
            const int oldestSampleIndex = static_cast<int>(frameStatistics.GetOldestSampleIndex());
            const std::span<const float> frameTimeSamples = frameStatistics.GetFrameTimeSamples();
            const float longestSample = frameTimeSamples.empty() ? 0.0f : std::ranges::max(frameTimeSamples);
            const float graphTop = std::max(MinimumGraphTopFrameTime, longestSample);
            const std::string graphCaption = std::format("0 - {:.0f} ms", graphTop * MillisecondsPerSecond);

            ImGui::PlotLines("##FrameTimes", frameTimeSamples.data(), static_cast<int>(frameTimeSamples.size()),
                             oldestSampleIndex, graphCaption.c_str(), 0.0f, graphTop,
                             ScaleToUI(ImVec2(GraphWidth, GraphHeight)));
        }
        ImGui::End();
    }

    std::string FormatFPSLimit(int maxFPS)
    {
        if (maxFPS == 0)
            return "Unlimited";

        return std::format("{} FPS", maxFPS);
    }
}
