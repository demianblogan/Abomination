#include "Audio/SoundStore.h"

#include "Core/Logging/Log.h"

#include <expected>
#include <optional>
#include <string>
#include <utility>

namespace Abomination::Audio
{
    using Core::LogCategory;
    using Core::LogLevel;

    SoundStore::SoundStore(std::filesystem::path assetsDirectory)
        : m_assetsDirectory(std::move(assetsDirectory))
        , m_fallbackClip(CreateFallbackBeep())
    {}

    SoundHandle SoundStore::Load(const std::string& path, Core::AssetLifetime lifetime)
    {
        if (const std::optional<SoundHandle> loadedHandle = m_cache.Find(path); loadedHandle.has_value())
        {
            m_cache.ExtendLifetime(*loadedHandle, lifetime);

            return *loadedHandle;
        }

        // make_preferred() turns the forward slashes of the asset path into the backslashes of Windows, so paths in log
        // messages do not mix both.
        std::filesystem::path fullPath = m_assetsDirectory / path;
        fullPath.make_preferred();

        std::expected<SoundClip, std::string> clip = DecodeSoundFile(fullPath);
        if (!clip.has_value())
        {
            Core::Log::Write(LogCategory::Audio, LogLevel::Warning, "Sound {} replaced by the fallback: {}", path,
                             clip.error());

            m_fallbackPaths.insert(path);

            return m_cache.Add(path, CreateFallbackBeep(), lifetime);
        }

        Core::Log::Write(LogCategory::Audio, LogLevel::Debug, "Sound loaded: {} ({} channels, {} Hz, {:.2f} s)", path,
                         clip->channelCount, clip->sampleRate,
                         static_cast<double>(clip->GetFrameCount()) / clip->sampleRate);

        return m_cache.Add(path, std::move(*clip), lifetime);
    }

    void SoundStore::RemoveAll(Core::AssetLifetime lifetime)
    {
        for (const std::string& path : m_cache.RemoveAll(lifetime))
            m_fallbackPaths.erase(path);
    }

    const SoundClip& SoundStore::Get(SoundHandle handle) const
    {
        const SoundClip* clip = m_cache.Get(handle);

        return clip != nullptr ? *clip : m_fallbackClip;
    }

    std::size_t SoundStore::GetCount() const noexcept
    {
        return m_cache.GetCount();
    }
}
