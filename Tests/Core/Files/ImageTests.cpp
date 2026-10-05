#include "Core/Files/Image.h"

#include <gtest/gtest.h>

// The tests write small PNG files with stb_image_write; its implementation is in Core/Files/Image.cpp.
#include <stb_image_write.h>

#include <array>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>

namespace Abomination::Core
{
    // Every test gets its own file path in the temporary folder; the file is removed afterwards.
    class ImageTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
            m_filePath = std::filesystem::temp_directory_path() / ("AbominationImageTest_" + testName + ".png");
        }

        void TearDown() override
        {
            std::filesystem::remove(m_filePath);
        }

        std::filesystem::path m_filePath;
    };

    TEST_F(ImageTest, LoadsPixelsWithBottomRowFirst)
    {
        // A 1x2 PNG with 3 channels (RGB): the top pixel is red, the bottom pixel is blue.
        constexpr std::array<std::uint8_t, 6> PixelsTopRowFirst{255, 0, 0, 0, 0, 255};
        stbi_write_png(m_filePath.string().c_str(), 1, 2, 3, PixelsTopRowFirst.data(), 3);

        const std::expected<Image, std::string> image = LoadImageFile(m_filePath);

        ASSERT_TRUE(image.has_value());
        EXPECT_EQ(image->width, 1);
        EXPECT_EQ(image->height, 2);
        ASSERT_EQ(image->pixels.size(), 8u);

        // The first stored row is the bottom one (blue); alpha is added and equals 255.
        EXPECT_EQ(image->pixels[0], 0);
        EXPECT_EQ(image->pixels[2], 255);
        EXPECT_EQ(image->pixels[3], 255);

        // The second stored row is the top one (red).
        EXPECT_EQ(image->pixels[4], 255);
        EXPECT_EQ(image->pixels[6], 0);
    }

    TEST_F(ImageTest, SavedImageLoadsWithTheSamePixels)
    {
        // 1x2, bottom row first like every Image: the bottom pixel green, the top one half-transparent red.
        const Image original{.width = 1, .height = 2, .pixels = {0, 255, 0, 255, 255, 0, 0, 128}};

        const std::expected<void, std::string> saved = SaveImageFile(m_filePath, original);
        const std::expected<Image, std::string> loaded = LoadImageFile(m_filePath);

        ASSERT_TRUE(saved.has_value());
        ASSERT_TRUE(loaded.has_value());
        EXPECT_EQ(loaded->width, 1);
        EXPECT_EQ(loaded->height, 2);
        EXPECT_EQ(loaded->pixels, original.pixels);
    }

    TEST_F(ImageTest, ReturnsErrorForMissingFile)
    {
        const std::expected<Image, std::string> image = LoadImageFile(m_filePath);

        ASSERT_FALSE(image.has_value());
        EXPECT_NE(image.error().find("Failed to open"), std::string::npos);
    }

    TEST_F(ImageTest, ReturnsErrorForFileThatIsNotImage)
    {
        std::ofstream(m_filePath) << "This is not a picture";

        const std::expected<Image, std::string> image = LoadImageFile(m_filePath);

        ASSERT_FALSE(image.has_value());
        EXPECT_NE(image.error().find("Failed to decode"), std::string::npos);
    }
}
