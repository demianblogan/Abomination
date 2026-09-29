#include "Core/Logging/LogHistory.h"

#include <utility>

namespace Abomination::Core
{
    LogHistory::LogHistory(std::size_t maxEntryCount)
        : m_maxEntryCount(maxEntryCount)
    {}

    void LogHistory::Add(LogEntry entry)
    {
        const std::lock_guard lock(m_mutex);

        if (m_maxEntryCount == 0)
            return;

        if (m_entries.size() == m_maxEntryCount)
            m_entries.pop_front();

        m_entries.push_back(std::move(entry));
        ++m_addedEntryCount;
    }

    void LogHistory::Clear()
    {
        const std::lock_guard lock(m_mutex);
        m_entries.clear();
    }

    std::size_t LogHistory::GetEntryCount() const
    {
        const std::lock_guard lock(m_mutex);

        return m_entries.size();
    }

    std::uint64_t LogHistory::GetAddedEntryCount() const
    {
        const std::lock_guard lock(m_mutex);

        return m_addedEntryCount;
    }
}
