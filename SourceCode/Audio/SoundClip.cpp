#include "Audio/SoundClip.h"

#include "Core/Files/FileSystem.h"

#include <miniaudio.h>

#include <cmath>
#include <cstddef>
#include <format>
#include <numbers>

namespace Abomination::Audio
{
    namespace
    {
        // The fallback beep: 0.15 s of a quiet 880 Hz tone (the A an octave above the tuning A), fading out at the end so
        // it does not click.
        constexpr std::uint32_t BeepSampleRate = 44100;
        constexpr float BeepFrequency = 880.0f;
        constexpr float BeepDuration = 0.15f;
        constexpr float BeepVolume = 0.3f;
    }

    std::expected<SoundClip, std::string> DecodeSoundFile(const std::filesystem::path& path)
    {
        // The file is read by our own function (it handles any path, including non-English characters), and miniaudio
        // only decodes the bytes in memory, like stb_image does for images.
        const std::expected<std::vector<std::byte>, std::string> fileContents = Core::ReadBinaryFile(path);
        if (!fileContents.has_value())
            return std::unexpected(fileContents.error());

        // The decoder recognizes the format by the contents and gives 32-bit float samples, mixed down to one channel (see
        // SoundClip::channelCount), in the sample rate of the file (0 means "keep what the file has").
        const ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 0);
        ma_decoder decoder;
        if (ma_decoder_init_memory(fileContents->data(), fileContents->size(), &config, &decoder) != MA_SUCCESS)
            return std::unexpected(std::format("unknown or broken sound format: {}", Core::ToUTF8String(path)));

        // A compressed format does not always know its length in advance, so the frames are read in blocks until the
        // decoder has no more.
        SoundClip clip{.channelCount = decoder.outputChannels, .sampleRate = decoder.outputSampleRate};
        constexpr ma_uint64 FramesPerBlock = 4096;
        while (true)
        {
            const std::size_t oldSampleCount = clip.samples.size();
            clip.samples.resize(oldSampleCount + FramesPerBlock * clip.channelCount);

            ma_uint64 framesRead = 0;
            const ma_result result =
                ma_decoder_read_pcm_frames(&decoder, clip.samples.data() + oldSampleCount, FramesPerBlock, &framesRead);

            // The last block is usually shorter: keep only what was read.
            clip.samples.resize(oldSampleCount + static_cast<std::size_t>(framesRead) * clip.channelCount);
            if (result != MA_SUCCESS || framesRead < FramesPerBlock)
                break;
        }

        ma_decoder_uninit(&decoder);

        if (clip.samples.empty())
            return std::unexpected(std::format("the sound has no samples: {}", Core::ToUTF8String(path)));

        return clip;
    }

    SoundClip CreateFallbackBeep()
    {
        const auto frameCount = static_cast<std::size_t>(BeepSampleRate * BeepDuration);

        SoundClip clip{.channelCount = 1, .sampleRate = BeepSampleRate};
        clip.samples.reserve(frameCount);
        for (std::size_t frame = 0; frame < frameCount; ++frame)
        {
            // A sine wave: sin() goes around once per period, BeepFrequency periods per second.
            const float time = static_cast<float>(frame) / static_cast<float>(BeepSampleRate);
            const float wave = std::sin(2.0f * std::numbers::pi_v<float> * BeepFrequency * time);

            // The volume falls from full to 0 over the beep: a sound that stops at full volume ends with a click.
            const float fade = 1.0f - static_cast<float>(frame) / static_cast<float>(frameCount);
            clip.samples.push_back(wave * fade * BeepVolume);
        }

        return clip;
    }
}
