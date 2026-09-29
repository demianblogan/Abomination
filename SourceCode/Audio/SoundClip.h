#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace Abomination::Audio
{
    // A sound decoded into memory, ready to be played any number of times at once.
    //
    // A sound file (OGG, WAV, MP3) is compressed or packed in its own way; playing it means turning it into samples:
    // numbers from -1 to 1 that say where the speaker membrane is at each moment. Here the whole file is decoded once,
    // when it is loaded, so playing a sound costs nothing but mixing. Short game sounds (a shot, a jump) take little
    // memory this way: one second of mono at 44100 Hz is 44100 floats, 172 KB. Music, minutes long, will be streamed
    // from the file instead (0.8).
    struct SoundClip
    {
        // The samples of all channels interleaved, frame after frame: for stereo left, right, left, right, ...
        // A frame is one sample of every channel, the sound of one moment.
        std::vector<float> samples;

        // 1 (mono) or 2 (stereo). DecodeSoundFile always gives mono, whatever the file has: a sound placed in the world
        // must come from one point, which a stereo sound cannot, and the short sounds of the game gain nothing from
        // stereo. Music, which does, will be streamed separately (0.8).
        std::uint32_t channelCount = 1;

        // Frames per second, for example 44100 or 48000. miniaudio converts it to the rate of the sound card.
        std::uint32_t sampleRate = 44100;

        [[nodiscard]] std::size_t GetFrameCount() const noexcept
        {
            return channelCount == 0 ? 0 : samples.size() / channelCount;
        }
    };

    // Decodes a whole sound file (OGG Vorbis, WAV, MP3 or FLAC) into mono samples; the channels of a stereo file are
    // mixed together.
    [[nodiscard]] std::expected<SoundClip, std::string> DecodeSoundFile(const std::filesystem::path& path);

    // A short high beep, used instead of a sound file that could not be loaded: like the magenta texture, it cannot be
    // overlooked, so a missing file is noticed in the game, not only in the log.
    [[nodiscard]] SoundClip CreateFallbackBeep();
}
