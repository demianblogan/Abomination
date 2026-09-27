#pragma once

#include "Core/Log.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Abomination::Core
{
    class LogHistory;
}

namespace Abomination::UI
{
    // The in-game console: the log as a strip along the bottom of the screen, over the game, like the console of Quake.
    // Opened with the ` key (the key left of 1) or View > Console; it stays open even while the rest of the debug overlay
    // is hidden. The messages come from Core::LogHistory, colored by level and filtered by level and category.
    class ConsoleWindow
    {
    public:
        void Draw(Core::LogHistory& history);

        void Toggle() noexcept;
        [[nodiscard]] bool IsOpen() const noexcept;

        // For the check mark of View > Console, which switches the console like Toggle().
        [[nodiscard]] bool* GetOpenFlag() noexcept;

    private:
        // One flag per level and per category (module): true = shown.
        using LevelFlags = std::array<bool, Core::LogLevelNames.size()>;
        using CategoryFlags = std::array<bool, Core::LogCategoryNames.size()>;

        // An array of flags that are all true, the starting value of the filters.
        template <typename Flags>
        static constexpr Flags MakeAllTrue()
        {
            Flags flags{};
            flags.fill(true);
            return flags;
        }

        void DrawToolbar(Core::LogHistory& history);
        void DrawMessages(const Core::LogHistory& history);

        bool m_isOpen = false;

        // How much of the game shows through (1 = nothing) and the part of the screen height the console takes.
        float m_opacity = 0.85f;
        float m_heightFraction = 0.4f;

        // Which levels and which categories (modules) are shown, all at first. The index is the numeric value of
        // Core::LogLevel or Core::LogCategory.
        LevelFlags m_visibleLevels = MakeAllTrue<LevelFlags>();
        CategoryFlags m_visibleCategories = MakeAllTrue<CategoryFlags>();

        // Keep the newest message in sight: scroll to the bottom when a message arrives. m_seenAddedCount is the count of
        // messages added (LogHistory::GetAddedCount) the last time the console was drawn.
        bool m_isAutoScrollEnabled = true;
        std::uint64_t m_seenAddedCount = 0;

        // The positions of the shown messages in the history, rebuilt every frame (kept to reuse the memory).
        std::vector<std::size_t> m_shownIndices;
    };
}
