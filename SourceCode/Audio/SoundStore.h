#pragma once

#include "Audio/SoundClip.h"
#include "Core/Assets/AssetCache.h"
#include "Core/Assets/AssetHandle.h"
#include "Core/Assets/AssetLifetime.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Abomination::Audio
{
    using SoundHandle = Core::AssetHandle<SoundClip>;

    // Loads sounds from files and keeps every sound exactly once, however many objects play it.
    //
    // Like the texture store: a missing or broken file does not stop the game. The sound is replaced by a short beep
    // (see CreateFallbackBeep) stored under the path of the missing file, so the file is looked for and reported once.
    //
    // Needs no sound card: it only decodes files, so tests can use it without an audio device.
    class SoundStore
    {
    public:
        // assetsDirectory: the folder all sound paths are relative to.
        explicit SoundStore(std::filesystem::path assetsDirectory);

        // Returns the sound loaded from path, loading it on the first call. path is relative to the assets directory
        // and uses forward slashes: "Sounds/Player/Jump.ogg". The same path always gives the same handle. The sound stays
        // loaded for the lifetime (the longer one if it is asked for again with another lifetime).
        [[nodiscard]] SoundHandle Load(const std::string& path, Core::AssetLifetime lifetime);

        // Removes every sound of the lifetime group; their handles become invalid. Whoever plays sounds must stop the
        // voices that play them first (see AudioEngine::RemoveSounds).
        void RemoveAll(Core::AssetLifetime lifetime);

        // The sound of the handle. An invalid handle (default-constructed or of a removed sound) gives the beep, so code
        // that plays sounds never has to check for nullptr.
        [[nodiscard]] const SoundClip& Get(SoundHandle handle) const;

        // Calls visitor(path, handle, clip, isFallback, lifetime) for every loaded sound; isFallback is true for a missing
        // or broken file replaced by the beep. For the Audio window of the debug overlay.
        template <typename Visitor>
        void VisitSounds(Visitor&& visitor) const;

        [[nodiscard]] std::size_t GetCount() const noexcept;

    private:
        std::filesystem::path m_assetsDirectory;
        Core::AssetCache<SoundClip> m_cache;

        // Paths whose file could not be loaded and which hold the beep instead.
        std::unordered_set<std::string> m_fallbackPaths;

        // Returned by Get() for invalid handles.
        SoundClip m_fallbackClip;
    };

    template <typename Visitor>
    void SoundStore::VisitSounds(Visitor&& visitor) const
    {
        m_cache.VisitAssets([&](const std::string& path, const SoundClip& clip, Core::AssetLifetime lifetime)
        {
            // The cache gives the path, not the handle; the path finds it (a lookup in a hash map, only for the window).
            visitor(path, *m_cache.Find(path), clip, m_fallbackPaths.contains(path), lifetime);
        });
    }
}
