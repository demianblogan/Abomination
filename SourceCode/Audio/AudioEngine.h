#pragma once

#include "Audio/SoundEvent.h"
#include "Audio/SoundGroup.h"
#include "Audio/SoundStore.h"
#include "Core/Assets/AssetLifetime.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Abomination::Audio
{
    // Which voice plays a sound started by AudioEngine::Play, to stop it later (a looping sound). It stays safe after the
    // sound ended and the voice plays something else: the start order no longer matches, and Stop() does nothing.
    struct VoiceId
    {
        std::size_t index = 0;
        std::uint64_t startOrder = 0;  // 0: no voice (the sound was not started)
    };

    // The sound of the game: the sound card, the loaded sounds and the voices that play them. Built on miniaudio, which
    // mixes all playing sounds in its own thread, so a slow frame never makes the sound stutter; the game only gives
    // commands ("play this", "the listener is here").
    //
    // A voice is one sound playing (like one channel of a mixing desk). There are VoiceCount of them; which one plays a
    // new sound is decided by ChooseVoice.
    //
    // A sound plays either in 2D, "in the head" of the player (their own jump, later their own shot: it must not move
    // to one ear when they turn), or in 3D at a position in the world. A 3D sound is louder in the ear turned to it and
    // quieter the farther it is from the listener, whom SetListener() puts at the eyes of the player every frame.
    //
    // Without a sound card (none installed, or it fails to start) the game runs silently: Play() does nothing and a
    // warning is logged once. Move-only; miniaudio does not appear in this header.
    class AudioEngine
    {
    public:
        static constexpr std::size_t VoiceCount = 32;

        // assetsDirectory: the folder all sound paths are relative to.
        explicit AudioEngine(const std::filesystem::path& assetsDirectory);
        ~AudioEngine();

        AudioEngine(AudioEngine&& other) noexcept;
        AudioEngine& operator=(AudioEngine&& other) noexcept;
        AudioEngine(const AudioEngine&) = delete;
        AudioEngine& operator=(const AudioEngine&) = delete;

        // Loads a sound (see SoundStore::Load).
        [[nodiscard]] SoundHandle LoadSound(const std::string& path, Core::AssetLifetime lifetime);

        // Loads the variants of a sound event, numbered from 1: "Sounds/Weapons/Hit" with 3 variants loads
        // Sounds/Weapons/Hit1.ogg, Hit2.ogg and Hit3.ogg, and keeps the event under that name, in the group. The other
        // values of the event are the defaults of SoundEvent; change them with GetSoundEvent. Loading a name again gives
        // the same handle and keeps the values already set (the longer lifetime wins, like for assets).
        [[nodiscard]] SoundEventHandle LoadSoundEvent(const std::string& pathWithoutNumber, int variantCount,
                                                      SoundGroup group, Core::AssetLifetime lifetime);

        // The event of the handle, to read or change its values, or nullptr for an invalid handle.
        [[nodiscard]] SoundEvent* GetSoundEvent(SoundEventHandle handle);
        [[nodiscard]] const SoundEvent* GetSoundEvent(SoundEventHandle handle) const;

        // Every loaded event, with its name, for the Audio window of the debug overlay.
        [[nodiscard]] std::vector<std::pair<std::string, SoundEventHandle>> ListSoundEvents() const;

        // Stops every voice and removes every sound and sound event of the lifetime group. The voices are stopped first: a
        // voice reads the samples of its sound while it plays, from the thread of miniaudio.
        void RemoveSounds(Core::AssetLifetime lifetime);

        [[nodiscard]] const SoundStore& GetSounds() const noexcept;

        // Plays one variant of the event, chosen at random, with a random pitch (see SoundEvent). position: where the
        // sound is in the world (game meters), or nothing for a 2D sound. A looping sound plays until Stop(). volumeScale
        // multiplies the volume of the event for this one play (a softer landing). Returns the voice, or an id without a
        // voice if nothing was started (no sound card, an invalid handle, no variants).
        VoiceId Play(SoundEventHandle event, std::optional<glm::vec3> position = std::nullopt, bool isLooping = false,
                     float volumeScale = 1.0f);

        // Plays an event that is not kept by the engine (a test sound of the debug overlay), the same way.
        VoiceId Play(const SoundEvent& event, std::optional<glm::vec3> position = std::nullopt, bool isLooping = false,
                     float volumeScale = 1.0f);

        // Stops the sound if the voice still plays it.
        void Stop(VoiceId voice);

        void StopAll();

        // Where the ears are: position and the direction the listener looks (not necessarily normalized), game axes
        // (+Y up). Called every frame with the view of the camera.
        void SetListener(const glm::vec3& position, const glm::vec3& forward);

        // The last values given to SetListener() (for debug tools that place sounds around the listener).
        [[nodiscard]] glm::vec3 GetListenerPosition() const noexcept;
        [[nodiscard]] glm::vec3 GetListenerForward() const noexcept;

        // The volume of everything, 0 (silence) to 1.
        void SetMasterVolume(float volume);
        [[nodiscard]] float GetMasterVolume() const noexcept;

        // The volume of a group of sounds, 0 to 1 (see SoundGroup).
        void SetGroupVolume(SoundGroup group, float volume);
        [[nodiscard]] float GetGroupVolume(SoundGroup group) const noexcept;

        // How many voices play a sound right now (for the Audio window).
        [[nodiscard]] std::size_t GetPlayingVoiceCount() const;

        // A sound card was found and started.
        [[nodiscard]] bool HasDevice() const noexcept;

    private:
        // The miniaudio objects live on the heap, behind this pointer: miniaudio keeps pointers to them in its thread,
        // so they must never move in memory, while AudioEngine itself is moved (out of Application::Create).
        struct Implementation;
        std::unique_ptr<Implementation> m_implementation;
    };
}
