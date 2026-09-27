#pragma once

#include "Core/Log.h"

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
    inline constexpr std::size_t DefaultLogHistoryCapacity = 2000;

    // The last messages of the log, in memory, for the in-game console. When it is full, a new message pushes out the
    // oldest one, so it never grows beyond its capacity.
    //
    // Log writes to it (see LogSettings::history) and the debug overlay reads it. Messages may be written from any
    // thread, so every access is guarded by a mutex: one thread at a time.
    class LogHistory
    {
    public:
        explicit LogHistory(std::size_t capacity = DefaultLogHistoryCapacity);

        void Add(LogEntry entry);
        void Clear();

        // Calls visitor(entry) for every message, from the oldest to the newest. The history stays locked meanwhile:
        // the visitor must not write to the log, or the thread would wait for itself forever.
        template <typename Visitor>
        void VisitEntries(Visitor&& visitor) const;

        [[nodiscard]] std::size_t GetCount() const;

        // How many messages were ever added (not reduced by pushing out or Clear()). The console scrolls to the bottom
        // when it changes.
        [[nodiscard]] std::uint64_t GetAddedCount() const;

    private:
        // mutable: const functions lock it too; locking changes the mutex, not what the history holds.
        mutable std::mutex m_mutex;

        // A deque removes from the front as cheaply as it adds to the back: the oldest message leaves in O(1).
        std::deque<LogEntry> m_entries;
        std::size_t m_capacity = 0;
        std::uint64_t m_addedCount = 0;
    };

    template <typename Visitor>
    void LogHistory::VisitEntries(Visitor&& visitor) const
    {
        // lock_guard locks the mutex now and unlocks it when it goes out of scope, even if the visitor throws.
        const std::lock_guard lock(m_mutex);
        for (const LogEntry& entry : m_entries)
            visitor(entry);
    }
}
