#include "Core/Logging/LogHistory.h"

#include <gtest/gtest.h>

#include <deque>
#include <string>
#include <vector>

namespace Abomination::Core
{
    namespace
    {
        LogEntry CreateEntry(const std::string& message)
        {
            return LogEntry{
                .timeText = "12:00:00.000", .level = LogLevel::Info, .category = LogCategory::Core, .message = message};
        }

        std::vector<std::string> GetMessages(const LogHistory& history)
        {
            std::vector<std::string> messages;
            history.ReadEntries([&messages](const std::deque<LogEntry>& entries)
            {
                for (const LogEntry& entry : entries)
                    messages.push_back(entry.message);
            });

            return messages;
        }
    }

    TEST(LogHistory, KeepsEntriesFromOldestToNewest)
    {
        LogHistory history;

        history.Add(CreateEntry("first"));
        history.Add(CreateEntry("second"));

        EXPECT_EQ(GetMessages(history), (std::vector<std::string>{"first", "second"}));
        EXPECT_EQ(history.GetEntryCount(), 2u);
    }

    TEST(LogHistory, FullHistoryDropsOldestEntry)
    {
        LogHistory history(2);

        history.Add(CreateEntry("first"));
        history.Add(CreateEntry("second"));
        history.Add(CreateEntry("third"));

        EXPECT_EQ(GetMessages(history), (std::vector<std::string>{"second", "third"}));
    }

    TEST(LogHistory, AddedCountKeepsGrowingWhenEntriesAreDroppedOrCleared)
    {
        LogHistory history(2);
        history.Add(CreateEntry("first"));
        history.Add(CreateEntry("second"));
        history.Add(CreateEntry("third"));

        history.Clear();

        EXPECT_EQ(history.GetEntryCount(), 0u);
        EXPECT_EQ(history.GetAddedEntryCount(), 3u);
    }
}
