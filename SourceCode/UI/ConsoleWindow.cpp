#include "UI/ConsoleWindow.h"

#include "Core/LogHistory.h"
#include "UI/UIScale.h"

#include <imgui.h>

#include <algorithm>
#include <deque>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace Abomination::UI
{
    namespace
    {
        // The color of the level tag ("[warning]"), in the order of Core::LogLevel, like the colored console of spdlog:
        // trace gray, debug cyan, info green, warning yellow, error red, critical magenta.
        constexpr std::array<ImVec4, Core::LogLevelNames.size()> LevelColors = {
            ImVec4(0.6f, 0.6f, 0.6f, 1.0f), ImVec4(0.35f, 0.85f, 0.9f, 1.0f), ImVec4(0.45f, 0.85f, 0.4f, 1.0f),
            ImVec4(1.0f, 0.8f, 0.25f, 1.0f), ImVec4(1.0f, 0.38f, 0.33f, 1.0f), ImVec4(1.0f, 0.35f, 1.0f, 1.0f),
        };

        // The time and the module are dimmed, so the eye goes to the level and the message.
        constexpr ImVec4 DimmedColor(0.55f, 0.55f, 0.55f, 1.0f);
        constexpr ImVec4 MessageColor(0.9f, 0.9f, 0.88f, 1.0f);

        // The widths of the sliders and the filter lists in pixels at 100% scale.
        constexpr float SliderWidth = 110.0f;
        constexpr float FilterComboWidth = 120.0f;

        // The console never gets smaller or bigger than this part of the screen.
        constexpr float MinimumHeightFraction = 0.15f;
        constexpr float MaximumHeightFraction = 0.9f;

        // A thin vertical line between groups of the toolbar.
        void DrawToolbarSeparator()
        {
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
        }

        // A label on the left of the next control; the control itself gets a hidden label ("##...").
        void DrawLabel(const char* text)
        {
            ImGui::TextUnformatted(text);
            ImGui::SameLine();
        }

        // What the closed filter list shows: "All", "None", the one chosen name or "3 of 6".
        std::string MakeFilterPreview(std::span<const std::string_view> names, std::span<const bool> isShown)
        {
            std::size_t shownCount = 0;
            std::size_t lastShown = 0;
            for (std::size_t index = 0; index < isShown.size(); ++index)
                if (isShown[index])
                {
                    ++shownCount;
                    lastShown = index;
                }

            if (shownCount == isShown.size())
                return "All";
            if (shownCount == 0)
                return "None";
            if (shownCount == 1)
                return std::string(names[lastShown]);

            return std::format("{} of {}", shownCount, isShown.size());
        }

        // A drop-down list of check boxes, one per name, with "All" on top. Clicking check boxes does not close the list
        // (only a click outside it does), so several can be switched at once.
        void DrawFilterCombo(const char* label, std::span<const std::string_view> names, std::span<bool> isShown)
        {
            DrawLabel(label);
            ImGui::SetNextItemWidth(ScaleToUI(FilterComboWidth));

            // The text after "##" is the ID only: the label is drawn on the left instead.
            const std::string id = std::string("##") + label;
            const std::string preview = MakeFilterPreview(names, isShown);
            if (!ImGui::BeginCombo(id.c_str(), preview.c_str()))
                return;

            // "All" is checked when every entry is; clicking it checks or unchecks all of them.
            bool areAllShown = std::ranges::all_of(isShown, [](bool isEntryShown) { return isEntryShown; });
            if (ImGui::Checkbox("All", &areAllShown))
                std::ranges::fill(isShown, areAllShown);
            ImGui::Separator();

            for (std::size_t index = 0; index < names.size(); ++index)
            {
                const std::string name(names[index]);
                ImGui::Checkbox(name.c_str(), &isShown[index]);
            }

            ImGui::EndCombo();
        }
    }

    void ConsoleWindow::Draw(Core::LogHistory& history)
    {
        if (!m_isOpen)
        {
            // Forget the messages seen, so the console scrolls to the newest one as soon as it opens again.
            m_seenAddedCount = 0;
            return;
        }

        // The console is glued to the bottom of the screen and as wide as it. The work area is the screen without the
        // main menu bar of the overlay (if it is shown).
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const float height = viewport->WorkSize.y * m_heightFraction;
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - height));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, height));
        ImGui::SetNextWindowBgAlpha(m_opacity);

        // No title bar, not movable or resizable by the mouse (the sliders change the size), and not saved to
        // DebugOverlay.ini: its place is always the same.
        constexpr ImGuiWindowFlags Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("Console", nullptr, Flags))
        {
            DrawToolbar(history);
            ImGui::Separator();
            DrawMessages(history);
        }
        ImGui::End();
    }

    void ConsoleWindow::Toggle() noexcept
    {
        m_isOpen = !m_isOpen;
    }

    bool ConsoleWindow::IsOpen() const noexcept
    {
        return m_isOpen;
    }

    bool* ConsoleWindow::GetOpenFlag() noexcept
    {
        return &m_isOpen;
    }

    void ConsoleWindow::DrawToolbar(Core::LogHistory& history)
    {
        // Filters (levels, modules) | Auto-scroll | Opacity, Height | Clear
        DrawFilterCombo("Levels", Core::LogLevelNames, m_visibleLevels);
        ImGui::SameLine();
        DrawFilterCombo("Modules", Core::LogCategoryNames, m_visibleCategories);

        DrawToolbarSeparator();
        ImGui::Checkbox("Auto-scroll", &m_isAutoScrollEnabled);

        DrawToolbarSeparator();
        DrawLabel("Opacity");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("##Opacity", &m_opacity, 0.2f, 1.0f, "%.2f");
        ImGui::SameLine();
        DrawLabel("Height");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("##Height", &m_heightFraction, MinimumHeightFraction, MaximumHeightFraction, "%.2f");

        DrawToolbarSeparator();
        if (ImGui::Button("Clear"))
            history.Clear();
    }

    void ConsoleWindow::DrawMessages(const Core::LogHistory& history)
    {
        // A child window with its own scroll bar fills the rest of the console; the toolbar above stays in place.
        if (!ImGui::BeginChild("Messages", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
        {
            ImGui::EndChild();
            return;
        }

        history.ReadEntries([this](const std::deque<Core::LogEntry>& entries)
        {
            // First the messages that pass the filters, by their position in the history.
            m_shownIndices.clear();
            for (std::size_t index = 0; index < entries.size(); ++index)
            {
                const Core::LogEntry& entry = entries[index];
                if (m_visibleLevels[std::to_underlying(entry.level)] && m_visibleCategories[std::to_underlying(entry.category)])
                    m_shownIndices.push_back(index);
            }

            // ImGuiListClipper asks only for the lines that fit into the visible part: with 2000 messages the console
            // still draws only the few dozen on the screen. It works out which from the line height and the scrolling.
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(m_shownIndices.size()));
            while (clipper.Step())
            {
                for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; ++line)
                {
                    const Core::LogEntry& entry = entries[m_shownIndices[static_cast<std::size_t>(line)]];
                    const std::string category(Core::LogCategoryNames[std::to_underlying(entry.category)]);
                    const std::string level(Core::LogLevelNames[std::to_underlying(entry.level)]);

                    // [12:03:41.512] [Renderer] [warning] Message, in three colors. SameLine(0, 0) continues the line right
                    // after the previous piece, without the usual gap between items. Warnings and worse color the message
                    // too, so they stand out in a long list.
                    // "%s" instead of putting the text into the format itself: a message with a % in it would be read as
                    // a format command.
                    const ImVec4& levelColor = LevelColors[std::to_underlying(entry.level)];
                    ImGui::TextColored(DimmedColor, "[%s] [%s] ", entry.timeText.c_str(), category.c_str());
                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::TextColored(levelColor, "[%s] ", level.c_str());
                    ImGui::SameLine(0.0f, 0.0f);
                    const bool isProblem = entry.level >= Core::LogLevel::Warning;
                    ImGui::TextColored(isProblem ? levelColor : MessageColor, "%s", entry.message.c_str());
                }
            }
        });

        // A new message arrived since the last frame: scroll to the very bottom, where it is.
        const std::uint64_t addedCount = history.GetAddedCount();
        if (m_isAutoScrollEnabled && addedCount != m_seenAddedCount)
            ImGui::SetScrollHereY(1.0f);
        m_seenAddedCount = addedCount;

        ImGui::EndChild();
    }
}
