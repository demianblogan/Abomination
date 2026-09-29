#include "Audio/SoundStore.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Abomination::Audio
{
    namespace
    {
        // Writes a value in little-endian byte order, the order of the WAV format.
        template <typename Value>
        void WriteLittleEndian(std::ofstream& file, Value value)
        {
            for (std::size_t byte = 0; byte < sizeof(Value); ++byte)
                file.put(static_cast<char>((static_cast<std::uint64_t>(value) >> (8 * byte)) & 0xFF));
        }

        // Writes the simplest WAV file: a 44-byte header and 16-bit mono samples.
        void WriteMonoWav(const std::filesystem::path& path, std::uint32_t sampleRate, const std::vector<std::int16_t>& samples)
        {
            const auto dataSize = static_cast<std::uint32_t>(samples.size() * sizeof(std::int16_t));

            std::ofstream file(path, std::ios::binary);
            file.write("RIFF", 4);
            WriteLittleEndian<std::uint32_t>(file, 36 + dataSize);  // the size of everything after this field
            file.write("WAVE", 4);
            file.write("fmt ", 4);
            WriteLittleEndian<std::uint32_t>(file, 16);              // the size of the format block
            WriteLittleEndian<std::uint16_t>(file, 1);               // 1: uncompressed samples (PCM)
            WriteLittleEndian<std::uint16_t>(file, 1);               // channels
            WriteLittleEndian<std::uint32_t>(file, sampleRate);
            WriteLittleEndian<std::uint32_t>(file, sampleRate * 2);  // bytes per second
            WriteLittleEndian<std::uint16_t>(file, 2);               // bytes per frame
            WriteLittleEndian<std::uint16_t>(file, 16);              // bits per sample
            file.write("data", 4);
            WriteLittleEndian<std::uint32_t>(file, dataSize);
            for (const std::int16_t sample : samples)
                WriteLittleEndian<std::uint16_t>(file, static_cast<std::uint16_t>(sample));
        }
    }

    // Every test gets its own folder in the temporary folder; it is removed afterwards.
    class SoundStoreTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
            m_directory = std::filesystem::temp_directory_path() / ("AbominationSoundStoreTest_" + testName);
            std::filesystem::create_directories(m_directory);
        }

        void TearDown() override
        {
            std::filesystem::remove_all(m_directory);
        }

        std::filesystem::path m_directory;
    };

    TEST_F(SoundStoreTest, DecodesWavIntoFloatSamples)
    {
        // Full positive, silence, full negative: 32767 and -32768 become about 1 and exactly -1.
        WriteMonoWav(m_directory / "Beep.wav", 22050, {32767, 0, -32768});
        SoundStore store(m_directory);

        const SoundClip& clip = store.Get(store.Load("Beep.wav", Core::AssetLifetime::Global));

        EXPECT_EQ(clip.channelCount, 1u);
        EXPECT_EQ(clip.sampleRate, 22050u);
        ASSERT_EQ(clip.GetFrameCount(), 3u);
        EXPECT_NEAR(clip.samples[0], 1.0f, 1e-3f);
        EXPECT_FLOAT_EQ(clip.samples[1], 0.0f);
        EXPECT_FLOAT_EQ(clip.samples[2], -1.0f);
    }

    TEST_F(SoundStoreTest, SamePathGivesSameHandle)
    {
        WriteMonoWav(m_directory / "Beep.wav", 22050, {0, 100});
        SoundStore store(m_directory);

        const SoundHandle first = store.Load("Beep.wav", Core::AssetLifetime::Level);
        const SoundHandle second = store.Load("Beep.wav", Core::AssetLifetime::Level);

        EXPECT_EQ(first, second);
        EXPECT_EQ(store.GetCount(), 1u);
    }

    TEST_F(SoundStoreTest, MissingFileGivesFallbackBeep)
    {
        SoundStore store(m_directory);

        const SoundHandle handle = store.Load("Missing.ogg", Core::AssetLifetime::Global);

        // The beep is stored under the path: a valid handle, a sound that plays, and the store knows it is a fallback.
        EXPECT_GT(store.Get(handle).GetFrameCount(), 0u);
        bool isFallback = false;
        store.VisitSounds([&](const std::string&, SoundHandle, const SoundClip&, bool fallback, Core::AssetLifetime)
        {
            isFallback = fallback;
        });
        EXPECT_TRUE(isFallback);
    }

    TEST_F(SoundStoreTest, RemovedSoundsGiveFallbackBeep)
    {
        WriteMonoWav(m_directory / "Beep.wav", 22050, {0, 100, 200});
        SoundStore store(m_directory);
        const SoundHandle handle = store.Load("Beep.wav", Core::AssetLifetime::Level);

        store.RemoveAll(Core::AssetLifetime::Level);

        EXPECT_EQ(store.GetCount(), 0u);
        EXPECT_EQ(store.Get(handle).GetFrameCount(), CreateFallbackBeep().GetFrameCount());
    }
}
