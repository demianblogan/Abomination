#include "Core/Files/FileSystem.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Abomination::Core
{
    // Every test gets its own file path in the temporary folder; the file is removed afterwards.
    class FileSystemTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
            m_filePath = std::filesystem::temp_directory_path() / ("AbominationFileSystemTest_" + testName);
        }

        void TearDown() override
        {
            std::filesystem::remove(m_filePath);
        }

        void WriteFile(const std::string& contents) const
        {
            std::ofstream file(m_filePath, std::ios::binary);
            file << contents;
        }

        std::filesystem::path m_filePath;
    };

    TEST_F(FileSystemTest, ReadsTextFileExactly)
    {
        // "\r\n" must stay as it is: the file is read without any conversion.
        WriteFile("#version 460 core\r\nvoid main() {}\n");

        const std::expected<std::string, std::string> contents = ReadTextFile(m_filePath);

        ASSERT_TRUE(contents.has_value());
        EXPECT_EQ(*contents, "#version 460 core\r\nvoid main() {}\n");
    }

    TEST_F(FileSystemTest, ReadsEmptyFile)
    {
        WriteFile("");

        const std::expected<std::string, std::string> contents = ReadTextFile(m_filePath);

        ASSERT_TRUE(contents.has_value());
        EXPECT_TRUE(contents->empty());
    }

    TEST_F(FileSystemTest, ReadsBinaryFileAsBytes)
    {
        WriteFile(std::string("\x89PNG\0\x01", 6));

        const std::expected<std::vector<std::byte>, std::string> contents = ReadBinaryFile(m_filePath);

        ASSERT_TRUE(contents.has_value());
        ASSERT_EQ(contents->size(), 6u);
        EXPECT_EQ((*contents)[0], std::byte{0x89});
        EXPECT_EQ((*contents)[4], std::byte{0x00});
        EXPECT_EQ((*contents)[5], std::byte{0x01});
    }

    TEST_F(FileSystemTest, ReturnsErrorForMissingFile)
    {
        const std::expected<std::string, std::string> contents = ReadTextFile(m_filePath);

        ASSERT_FALSE(contents.has_value());
        EXPECT_NE(contents.error().find("Failed to open"), std::string::npos);
    }

    TEST(FileSystem, PathWithCyrillicLettersBecomesUTF8)
    {
        // "Папка/Файл.png": every Cyrillic letter is two bytes in UTF-8 (П is D0 9F), whatever the code page of Windows.
        const std::filesystem::path path(u8"Папка/Файл.png");

        const std::string text = ToUTF8String(path);

        EXPECT_EQ(text.size(), 2u * 5u + 1u + 2u * 4u + 4u); // 5 and 4 letters, "/" and ".png"
        EXPECT_EQ(text.substr(0, 2), "\xD0\x9F");
        EXPECT_EQ(text.substr(text.size() - 4), ".png");
    }

    TEST(FileSystem, ErrorMessageShowsPathInUTF8)
    {
        const std::filesystem::path missingFile(u8"Нет.txt"); // "Нет.txt"

        const std::expected<std::string, std::string> contents = ReadTextFile(missingFile);

        ASSERT_FALSE(contents.has_value());
        EXPECT_NE(contents.error().find(ToUTF8String(missingFile)), std::string::npos);
    }
}
