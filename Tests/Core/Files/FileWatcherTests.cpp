#include "Core/Files/FileWatcher.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Abomination::Core
{
    namespace
    {
        // Two shader files in a folder of their own. The write times are set explicitly, so the tests do not depend on how
        // precise the clock of the file system is.
        class FileWatcherTest : public ::testing::Test
        {
        protected:
            void SetUp() override
            {
                m_directory = std::filesystem::temp_directory_path() / "AbominationFileWatcherTest";
                std::filesystem::remove_all(m_directory);
                std::filesystem::create_directories(m_directory);
                m_baseTime = std::filesystem::file_time_type::clock::now() - std::chrono::hours(1);
                Write(VertexPath(), "vertex");
                Write(FragmentPath(), "fragment");
                SetWriteTime(VertexPath(), 0);
                SetWriteTime(FragmentPath(), 0);
            }

            void TearDown() override
            {
                std::filesystem::remove_all(m_directory);
            }

            std::filesystem::path VertexPath() const
            {
                return m_directory / "Test.vert";
            }

            std::filesystem::path FragmentPath() const
            {
                return m_directory / "Test.frag";
            }

            static void Write(const std::filesystem::path& path, const std::string& text)
            {
                std::ofstream(path) << text;
            }

            // A write time some seconds after a fixed moment.
            void SetWriteTime(const std::filesystem::path& path, int seconds) const
            {
                std::filesystem::last_write_time(path, m_baseTime + std::chrono::seconds(seconds));
            }

            std::filesystem::path m_directory;
            std::filesystem::file_time_type m_baseTime;
        };
    }

    TEST_F(FileWatcherTest, NothingChangedReportsNothing)
    {
        FileWatcher watcher;
        watcher.Watch("Test", {VertexPath(), FragmentPath()});

        EXPECT_TRUE(watcher.CollectChangedKeys().empty());
        EXPECT_TRUE(watcher.CollectChangedKeys().empty());
    }

    TEST_F(FileWatcherTest, ChangeIsReportedOnceAfterItStops)
    {
        FileWatcher watcher;
        watcher.Watch("Test", {VertexPath(), FragmentPath()});

        SetWriteTime(FragmentPath(), 10);

        // The first check only sees the new time: the file may still be being written.
        EXPECT_TRUE(watcher.CollectChangedKeys().empty());
        // The second sees the same time: the change is finished.
        EXPECT_EQ(watcher.CollectChangedKeys(), std::vector<std::string>{"Test"});
        // Reported once.
        EXPECT_TRUE(watcher.CollectChangedKeys().empty());
    }

    TEST_F(FileWatcherTest, FileStillChangingIsNotReported)
    {
        FileWatcher watcher;
        watcher.Watch("Test", {VertexPath(), FragmentPath()});

        SetWriteTime(VertexPath(), 10);
        EXPECT_TRUE(watcher.CollectChangedKeys().empty());
        SetWriteTime(VertexPath(), 20);
        EXPECT_TRUE(watcher.CollectChangedKeys().empty());

        EXPECT_EQ(watcher.CollectChangedKeys(), std::vector<std::string>{"Test"});
    }

    TEST_F(FileWatcherTest, BothFilesChangedReportTheKeyOnce)
    {
        FileWatcher watcher;
        watcher.Watch("Test", {VertexPath(), FragmentPath()});

        SetWriteTime(VertexPath(), 10);
        SetWriteTime(FragmentPath(), 10);
        static_cast<void>(watcher.CollectChangedKeys());

        EXPECT_EQ(watcher.CollectChangedKeys(), std::vector<std::string>{"Test"});
    }

    TEST_F(FileWatcherTest, MissingFileIsSkippedUntilItIsBack)
    {
        FileWatcher watcher;
        watcher.Watch("Test", {VertexPath(), FragmentPath()});

        std::filesystem::remove(FragmentPath());
        EXPECT_TRUE(watcher.CollectChangedKeys().empty());
        EXPECT_TRUE(watcher.CollectChangedKeys().empty());

        // Saved again, as an editor does: a new file with a new time.
        Write(FragmentPath(), "fragment, fixed");
        SetWriteTime(FragmentPath(), 30);
        static_cast<void>(watcher.CollectChangedKeys());
        EXPECT_EQ(watcher.CollectChangedKeys(), std::vector<std::string>{"Test"});
    }
}
