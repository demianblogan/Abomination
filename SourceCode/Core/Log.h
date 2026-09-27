#pragma once

#include <array>
#include <filesystem>
#include <format>
#include <string_view>
#include <utility>

namespace Abomination::Core
{
    class LogHistory;

    // Ordered from the least to the most important. Messages below LogSettings::minimumLevel are discarded.
    enum class LogLevel
    {
        Trace,    // Step-by-step details, often every frame. Enabled only while debugging one system
        Debug,    // Useful for the developer, but not every frame
        Info,     // Important normal events: a short history of the run
        Warning,  // Something went wrong, but the game handled it
        Error,    // An operation failed, the game keeps running
        Critical, // The game cannot continue and is about to close
    };

    // The module a message comes from. A new category is added with a new module.
    enum class LogCategory
    {
        Core,
        Platform,
        Renderer,
        UI,
        World,
    };

    // Names for tools (the in-game console), in the order of the enum values. The log file uses the same names.
    inline constexpr std::array<std::string_view, 6> LogLevelNames = {
        "trace", "debug", "info", "warning", "error", "critical"};
    inline constexpr std::array<std::string_view, 5> LogCategoryNames = {"Core", "Platform", "Renderer", "UI", "World"};

    struct LogSettings
    {
        std::filesystem::path filePath = "Abomination.log";
        LogLevel minimumLevel = LogLevel::Info;
        bool needToWriteToConsole = true;

        // If set, every written message is also kept here (see LogHistory), for the in-game console. It must stay alive
        // until Shutdown().
        LogHistory* history = nullptr;
    };
}

// Implementation details used by the Log::Write template. Do not call directly.
namespace Abomination::Core::Log::Internal
{
    // True when messages of this level are written (the level is not below the minimum level).
    [[nodiscard]] bool IsLevelEnabled(LogLevel level) noexcept;

    void WriteMessage(LogCategory category, LogLevel level, std::string_view message);
}

// Logging for the whole game. spdlog is used internally and never included elsewhere.
// Messages written before Initialize() or after Shutdown() are ignored.
// Initialize() and Shutdown() are not thread-safe; Write() is.
namespace Abomination::Core::Log
{
    // Starts logging. Calling it again restarts logging with the new settings.
    void Initialize(const LogSettings& settings);

    // Flushes all messages to the file and stops logging.
    void Shutdown();

    // Formats and writes a message. The format string is checked at compile time.
    // Formatting is skipped when the level is disabled, so disabled messages cost almost nothing.
    template <typename... Arguments>
    void Write(LogCategory category, LogLevel level, std::format_string<Arguments...> format, Arguments&&... arguments)
    {
        if (Internal::IsLevelEnabled(level))
            Internal::WriteMessage(category, level, std::format(format, std::forward<Arguments>(arguments)...));
    }
}
