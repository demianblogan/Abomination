#include "Core/Log.h"
#include "Core/LogHistory.h"

#include <gtest/gtest.h>

#include <deque>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace Abomination::Core
{
    // Every test writes to its own log file in the temporary folder and removes it afterwards.
    class LogTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
            m_logFilePath = std::filesystem::temp_directory_path() / ("AbominationLogTest_" + testName + ".log");
        }

        void TearDown() override
        {
            Log::Shutdown();
            std::filesystem::remove(m_logFilePath);
        }

        void InitializeLog(LogLevel minimumLevel) const
        {
            Log::Initialize(LogSettings{
                .filePath = m_logFilePath,
                .minimumLevel = minimumLevel,
            });
        }

        // Stops logging, so all messages are flushed and the file is closed, then returns the file contents.
        std::string ShutdownAndReadLogFile() const
        {
            Log::Shutdown();

            std::ifstream file(m_logFilePath);
            std::stringstream contents;
            contents << file.rdbuf();

            return contents.str();
        }

        std::filesystem::path m_logFilePath;
    };

    TEST_F(LogTest, WritesFormattedMessageWithCategoryAndLevel)
    {
        InitializeLog(LogLevel::Trace);

        Log::Write(LogCategory::Renderer, LogLevel::Warning, "Texture {} not found", "Crate.png");

        EXPECT_NE(ShutdownAndReadLogFile().find("[Renderer] [warning] Texture Crate.png not found"), std::string::npos);
    }

    TEST_F(LogTest, DiscardsMessagesBelowMinimumLevel)
    {
        InitializeLog(LogLevel::Info);

        Log::Write(LogCategory::Core, LogLevel::Debug, "Hidden message");
        Log::Write(LogCategory::Core, LogLevel::Info, "Visible message");

        const std::string contents = ShutdownAndReadLogFile();
        EXPECT_EQ(contents.find("Hidden message"), std::string::npos);
        EXPECT_NE(contents.find("Visible message"), std::string::npos);
    }

    TEST_F(LogTest, IgnoresMessagesAfterShutdown)
    {
        InitializeLog(LogLevel::Trace);
        Log::Write(LogCategory::Core, LogLevel::Info, "Before shutdown");
        Log::Shutdown();

        Log::Write(LogCategory::Core, LogLevel::Info, "After shutdown");

        const std::string contents = ShutdownAndReadLogFile();
        EXPECT_NE(contents.find("Before shutdown"), std::string::npos);
        EXPECT_EQ(contents.find("After shutdown"), std::string::npos);
    }

    TEST_F(LogTest, InitializingAgainStartsWithEmptyFile)
    {
        InitializeLog(LogLevel::Trace);
        Log::Write(LogCategory::Core, LogLevel::Info, "First run");

        InitializeLog(LogLevel::Trace);
        Log::Write(LogCategory::Core, LogLevel::Info, "Second run");

        const std::string contents = ShutdownAndReadLogFile();
        EXPECT_EQ(contents.find("First run"), std::string::npos);
        EXPECT_NE(contents.find("Second run"), std::string::npos);
    }

    TEST_F(LogTest, HistoryKeepsMessagesInParts)
    {
        LogHistory history;
        Log::Initialize(LogSettings{
            .filePath = m_logFilePath,
            .minimumLevel = LogLevel::Info,
            .history = &history,
        });

        Log::Write(LogCategory::World, LogLevel::Warning, "The map has no {}", "info_player_start");
        Log::Write(LogCategory::Core, LogLevel::Debug, "Below the minimum level");
        Log::Shutdown();

        std::vector<LogEntry> entries;
        history.ReadEntries([&entries](const std::deque<LogEntry>& historyEntries)
        {
            entries.assign(historyEntries.begin(), historyEntries.end());
        });

        ASSERT_EQ(entries.size(), 1u);
        EXPECT_EQ(entries[0].category, LogCategory::World);
        EXPECT_EQ(entries[0].level, LogLevel::Warning);
        EXPECT_EQ(entries[0].message, "The map has no info_player_start");
        EXPECT_EQ(entries[0].timeText.size(), std::string("12:03:41.512").size());
    }
}
