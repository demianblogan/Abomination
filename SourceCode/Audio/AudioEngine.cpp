#include "Audio/AudioEngine.h"

#include "Audio/VoiceSelection.h"
#include "Core/Logging/Log.h"
#include "Core/Math/Random.h"

#include <miniaudio.h>

#include <algorithm>
#include <array>
#include <utility>

namespace Abomination::Audio
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // How a 3D sound gets quieter with distance: at full volume up to MinimumDistance, then at half the volume at
        // twice that distance, a third at three times, ... (the "inverse" model, like in the real world), never quieter
        // than at MaximumDistance. Game meters.
        constexpr float MinimumDistance = 2.0f;
        constexpr float MaximumDistance = 50.0f;

        // One voice: a sound playing (or a free place for one).
        struct Voice
        {
            // A data source that reads the samples of a SoundClip where they are, without copying them. The clip must
            // stay in memory while the voice plays (see AudioEngine::RemoveSounds).
            ma_audio_buffer_ref buffer{};

            // The playing sound: volume, pitch, position, and the mixing in miniaudio's thread.
            ma_sound sound{};

            // buffer and sound are set up and must be released.
            bool isInitialized = false;

            SoundHandle eventKey;
            std::uint64_t startOrder = 0;
        };

        void ReleaseVoice(Voice& voice)
        {
            if (!voice.isInitialized)
                return;

            // ma_sound_uninit() also stops the sound and waits until miniaudio's thread no longer reads it.
            ma_sound_uninit(&voice.sound);
            ma_audio_buffer_ref_uninit(&voice.buffer);
            voice.isInitialized = false;
        }
    }

    struct AudioEngine::Implementation
    {
        explicit Implementation(const std::filesystem::path& assetsDirectory)
            : sounds(assetsDirectory)
        {}

        ~Implementation()
        {
            for (Voice& voice : voices)
                ReleaseVoice(voice);

            if (hasDevice)
                ma_engine_uninit(&engine);
        }

        // Neither copied nor moved: miniaudio points into it (see m_implementation).
        Implementation(const Implementation&) = delete;
        Implementation& operator=(const Implementation&) = delete;

        SoundStore sounds;

        // The miniaudio engine: the sound card, the mixer, the listener.
        ma_engine engine{};
        bool hasDevice = false;

        std::array<Voice, VoiceCount> voices{};

        // Grows with every started voice (see VoiceInfo::startOrder); 0 means "no voice".
        std::uint64_t nextStartOrder = 1;

        float masterVolume = 1.0f;

        // The last listener given to SetListener(); looks along -Z like a new miniaudio listener.
        glm::vec3 listenerPosition{0.0f};
        glm::vec3 listenerForward{0.0f, 0.0f, -1.0f};

        // Chooses the variants and the pitch.
        Core::Random random;
    };

    AudioEngine::AudioEngine(const std::filesystem::path& assetsDirectory)
        : m_implementation(std::make_unique<Implementation>(assetsDirectory))
    {
        // The default configuration opens the default sound card of the system, in its own format.
        if (ma_engine_init(nullptr, &m_implementation->engine) != MA_SUCCESS)
        {
            Core::Log::Write(LogCategory::Audio, LogLevel::Warning, "No sound card could be started: the game runs silently");

            return;
        }

        m_implementation->hasDevice = true;
        Core::Log::Write(LogCategory::Audio, LogLevel::Info, "Audio started: {} Hz, {} channels",
                         ma_engine_get_sample_rate(&m_implementation->engine),
                         ma_engine_get_channels(&m_implementation->engine));
    }

    AudioEngine::~AudioEngine() = default;
    AudioEngine::AudioEngine(AudioEngine&& other) noexcept = default;
    AudioEngine& AudioEngine::operator=(AudioEngine&& other) noexcept = default;

    SoundHandle AudioEngine::LoadSound(const std::string& path, Core::AssetLifetime lifetime)
    {
        return m_implementation->sounds.Load(path, lifetime);
    }

    SoundEvent AudioEngine::LoadSoundEvent(const std::string& pathWithoutNumber, int variantCount,
                                           Core::AssetLifetime lifetime)
    {
        SoundEvent event;
        for (int variant = 1; variant <= variantCount; ++variant)
            event.variants.push_back(LoadSound(pathWithoutNumber + std::to_string(variant) + ".ogg", lifetime));

        return event;
    }

    void AudioEngine::RemoveSounds(Core::AssetLifetime lifetime)
    {
        StopAll();
        m_implementation->sounds.RemoveAll(lifetime);
    }

    const SoundStore& AudioEngine::GetSounds() const noexcept
    {
        return m_implementation->sounds;
    }

    VoiceId AudioEngine::Play(const SoundEvent& event, std::optional<glm::vec3> position, bool isLooping)
    {
        Implementation& state = *m_implementation;
        if (!state.hasDevice || event.variants.empty())
            return {};

        // The voice: an event at its limit replaces its oldest copy, otherwise a free voice or the oldest of all.
        std::array<VoiceInfo, VoiceCount> voiceInfos;
        for (std::size_t index = 0; index < VoiceCount; ++index)
        {
            const Voice& voice = state.voices[index];
            voiceInfos[index] = {
                .isPlaying = voice.isInitialized && ma_sound_is_playing(&voice.sound),
                .eventKey = voice.eventKey,
                .startOrder = voice.startOrder,
            };
        }
        const std::size_t voiceIndex = ChooseVoice(voiceInfos, event.variants.front(), event.maxVoices);
        Voice& voice = state.voices[voiceIndex];
        ReleaseVoice(voice);

        const SoundHandle variant = event.variants[state.random.GetIndex(event.variants.size())];
        const SoundClip& clip = state.sounds.Get(variant);

        // The data source reads the samples of the clip in place. sampleRate is set after init: the init function of
        // this version has no parameter for it, and without it miniaudio would assume the rate of the sound card.
        ma_audio_buffer_ref_init(ma_format_f32, clip.channelCount, clip.samples.data(), clip.GetFrameCount(), &voice.buffer);
        voice.buffer.sampleRate = clip.sampleRate;

        // A 2D sound skips spatialization: it plays as recorded, whatever the listener does.
        const ma_uint32 flags = position.has_value() ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;
        if (ma_sound_init_from_data_source(&state.engine, &voice.buffer, flags, nullptr, &voice.sound) != MA_SUCCESS)
        {
            ma_audio_buffer_ref_uninit(&voice.buffer);
            Core::Log::Write(LogCategory::Audio, LogLevel::Warning, "A sound could not be started");

            return {};
        }

        voice.isInitialized = true;
        voice.eventKey = event.variants.front();
        voice.startOrder = state.nextStartOrder++;

        ma_sound_set_volume(&voice.sound, event.volume);
        ma_sound_set_pitch(&voice.sound, 1.0f + state.random.GetFloat(-event.pitchVariation, event.pitchVariation));
        ma_sound_set_looping(&voice.sound, isLooping ? MA_TRUE : MA_FALSE);
        if (position.has_value())
        {
            ma_sound_set_position(&voice.sound, position->x, position->y, position->z);
            ma_sound_set_min_distance(&voice.sound, MinimumDistance);
            ma_sound_set_max_distance(&voice.sound, MaximumDistance);
        }

        ma_sound_start(&voice.sound);

        return {.index = voiceIndex, .startOrder = voice.startOrder};
    }

    void AudioEngine::Stop(VoiceId voiceId)
    {
        if (voiceId.startOrder == 0 || voiceId.index >= VoiceCount)
            return;

        Voice& voice = m_implementation->voices[voiceId.index];
        if (voice.startOrder == voiceId.startOrder)
            ReleaseVoice(voice);
    }

    void AudioEngine::StopAll()
    {
        for (Voice& voice : m_implementation->voices)
            ReleaseVoice(voice);
    }

    void AudioEngine::SetListener(const glm::vec3& position, const glm::vec3& forward)
    {
        m_implementation->listenerPosition = position;
        m_implementation->listenerForward = forward;
        if (!m_implementation->hasDevice)
            return;

        // miniaudio uses the same axes as OpenGL and the game (right-handed, +Y up, a listener looks along -Z by
        // default), so the vectors are passed as they are. 0 is the index of the listener: there is only one.
        ma_engine* engine = &m_implementation->engine;
        ma_engine_listener_set_position(engine, 0, position.x, position.y, position.z);
        ma_engine_listener_set_direction(engine, 0, forward.x, forward.y, forward.z);
        ma_engine_listener_set_world_up(engine, 0, 0.0f, 1.0f, 0.0f);
    }

    glm::vec3 AudioEngine::GetListenerPosition() const noexcept
    {
        return m_implementation->listenerPosition;
    }

    glm::vec3 AudioEngine::GetListenerForward() const noexcept
    {
        return m_implementation->listenerForward;
    }

    void AudioEngine::SetMasterVolume(float volume)
    {
        m_implementation->masterVolume = std::clamp(volume, 0.0f, 1.0f);
        if (m_implementation->hasDevice)
            ma_engine_set_volume(&m_implementation->engine, m_implementation->masterVolume);
    }

    float AudioEngine::GetMasterVolume() const noexcept
    {
        return m_implementation->masterVolume;
    }

    std::size_t AudioEngine::GetPlayingVoiceCount() const
    {
        return static_cast<std::size_t>(std::ranges::count_if(m_implementation->voices, [](const Voice& voice)
        {
            return voice.isInitialized && ma_sound_is_playing(&voice.sound);
        }));
    }

    bool AudioEngine::HasDevice() const noexcept
    {
        return m_implementation->hasDevice;
    }
}
