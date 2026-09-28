#include "Core/Logging/LogHistory.h"

#include <utility>

namespace Abomination::Core
{
    LogHistory::LogHistory(std::size_t capacity)
        : m_capacity(capacity)
    {}

    void LogHistory::Add(LogEntry entry)
    {
        const std::lock_guard lock(m_mutex);

        if (m_capacity == 0)
            return;

        if (m_entries.size() == m_capacity)
            m_entries.pop_front();

        m_entries.push_back(std::move(entry));
        ++m_addedCount;
    }

    void LogHistory::Clear()
    {
        const std::lock_guard lock(m_mutex);
        m_entries.clear();
    }

    std::size_t LogHistory::GetCount() const
    {
        const std::lock_guard lock(m_mutex);

        return m_entries.size();
    }

    std::uint64_t LogHistory::GetAddedCount() const
    {
        const std::lock_guard lock(m_mutex);

        return m_addedCount;
    }
}
