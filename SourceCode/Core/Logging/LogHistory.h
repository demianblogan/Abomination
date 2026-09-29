#pragma once

#include "Core/Logging/Log.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

namespace Abomination::Core
{
    // One message of the log, kept in parts, so the in-game console can filter by level and category.
    struct LogEntry
    {
        // The time as the log file shows it: "12:03:41.512".
        std::string timeText;
        LogLevel level = LogLevel::Info;
        LogCategory category = LogCategory::Core;
        std::string message;
    };

    // How many messages the history keeps by default: a few minutes of a normal run.
    inline constexpr std::size_t DefaultMaxLogEntryCount = 2000;

    // The last messages of the log, in memory, for the in-game console. When it holds maxEntryCount messages, a new one
    // pushes out the oldest one, so it never grows beyond that.
    //
    // Log writes to it (see LogSettings::history) and the debug overlay reads it. Messages may be written from any
    // thread, so every access is guarded by a mutex: one thread at a time.
    class LogHistory
    {
    public:
        explicit LogHistory(std::size_t maxEntryCount = DefaultMaxLogEntryCount);

        void Add(LogEntry entry);
        void Clear();

        // Calls reader(entries) once with all messages (a const std::deque<LogEntry>&, the oldest first) while the history
        // is locked: for readers that need to jump around in them, like the console, which draws only the visible lines.
        // The reader must not write to the log meanwhile, or the thread would wait for itself forever.
        template <typename Reader>
        void ReadEntries(Reader&& reader) const;

        [[nodiscard]] std::size_t GetEntryCount() const;

        // How many messages were ever added (not reduced by pushing out or Clear()). The console scrolls to the bottom
        // when it changes.
        [[nodiscard]] std::uint64_t GetAddedEntryCount() const;

    private:
        // mutable: const functions lock it too; locking changes the mutex, not what the history holds.
        mutable std::mutex m_mutex;

        // A deque removes from the front as cheaply as it adds to the back: the oldest message leaves in O(1).
        std::deque<LogEntry> m_entries;

        // The most messages m_entries holds at once.
        std::size_t m_maxEntryCount = 0;

        // How many messages were ever added (see GetAddedEntryCount).
        std::uint64_t m_addedEntryCount = 0;
    };

    template <typename Reader>
    void LogHistory::ReadEntries(Reader&& reader) const
    {
        // lock_guard locks the mutex now and unlocks it when it goes out of scope, even if the reader throws.
        const std::lock_guard lock(m_mutex);
        reader(m_entries); // const here, because the function is const
    }
}
