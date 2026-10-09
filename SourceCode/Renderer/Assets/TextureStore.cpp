#include "Renderer/Assets/TextureStore.h"

#include "Core/Files/Image.h"
#include "Core/Logging/Log.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

namespace Abomination::Renderer
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // The checkerboard is 8x8 texels, one texel per square. With the crisp GL_NEAREST filtering of our textures,
        // a face showing it has 64 sharp squares, whatever the size of the face.
        constexpr int FallbackTextureSize = 8;

        // Creates the magenta and black checkerboard in memory.
        GLTexture CreateFallbackTexture()
        {
            Core::Image image;
            image.width = FallbackTextureSize;
            image.height = FallbackTextureSize;
            constexpr int ByteCount = FallbackTextureSize * FallbackTextureSize * Core::ImageChannelCount;
            image.pixels.reserve(static_cast<std::size_t>(ByteCount));

            for (int y = 0; y < FallbackTextureSize; ++y)
            {
                for (int x = 0; x < FallbackTextureSize; ++x)
                {
                    // (x + y) is even and odd in turn along every row and every column: a checkerboard.
                    const bool isMagentaSquare = (x + y) % 2 == 0;
                    const std::uint8_t redAndBlue = isMagentaSquare ? 255 : 0;
                    image.pixels.insert(image.pixels.end(), {redAndBlue, 0, redAndBlue, 255});
                }
            }

            return GLTexture::CreateFromImage(image);
        }

        // An image of one texel of this color.
        Core::Image CreateTexel(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
        {
            return Core::Image{.width = 1, .height = 1, .pixels = {red, green, blue, 255}};
        }
    }

    TextureStore::TextureStore(std::filesystem::path assetsDirectory)
        : m_assetsDirectory(std::move(assetsDirectory))
        , m_fallbackTexture(CreateFallbackTexture())
    {
        // Raw: the numbers are data (a normal, a factor), not sRGB colors. For white and black it makes no difference.
        // 128 is the middle of 0..255: a normal of 0 along the axes of the texture, as near as 8 bits get to it.
        constexpr auto Lifetime = Core::AssetLifetime::Global;
        m_builtInTextures = {
            Add("Built-in/White", CreateTexel(255, 255, 255), Lifetime, TextureEncoding::Raw),
            Add("Built-in/Black", CreateTexel(0, 0, 0), Lifetime, TextureEncoding::Raw),
            Add("Built-in/FlatNormal", CreateTexel(128, 128, 255), Lifetime, TextureEncoding::Raw),
        };
    }

    TextureHandle TextureStore::Load(const std::string& path, Core::AssetLifetime lifetime, TextureEncoding encoding,
                                     TextureFiltering filtering)
    {
        if (const std::optional<TextureHandle> loadedHandle = m_cache.Find(path); loadedHandle.has_value())
        {
            m_cache.ExtendLifetime(*loadedHandle, lifetime);

            return *loadedHandle;
        }

        // make_preferred() turns the forward slashes of the asset path into the backslashes of Windows,
        // so paths in log messages do not mix both.
        std::filesystem::path fullPath = m_assetsDirectory / path;
        fullPath.make_preferred();

        std::expected<GLTexture, std::string> texture = GLTexture::CreateFromFile(fullPath, filtering, encoding);
        if (!texture.has_value())
        {
            Core::Log::Write(LogCategory::Renderer, LogLevel::Warning, "Texture {} replaced by the fallback: {}", path,
                             texture.error());

            m_fallbackPaths.insert(path);

            return m_cache.Add(path, CreateFallbackTexture(), lifetime);
        }

        Core::Log::Write(LogCategory::Renderer, LogLevel::Debug, "Texture loaded: {}", path);

        return m_cache.Add(path, std::move(*texture), lifetime);
    }

    TextureHandle TextureStore::Add(const std::string& name, const Core::Image& image, Core::AssetLifetime lifetime,
                                    TextureEncoding encoding, TextureFiltering filtering)
    {
        if (const std::optional<TextureHandle> loadedHandle = m_cache.Find(name); loadedHandle.has_value())
        {
            m_cache.ExtendLifetime(*loadedHandle, lifetime);

            return *loadedHandle;
        }

        if (image.width == 0 || image.height == 0)
        {
            Core::Log::Write(LogCategory::Renderer, LogLevel::Warning, "Texture {} replaced by the fallback: no image", name);
            m_fallbackPaths.insert(name);

            return m_cache.Add(name, CreateFallbackTexture(), lifetime);
        }

        Core::Log::Write(LogCategory::Renderer, LogLevel::Debug, "Texture added: {} ({}x{})", name, image.width, image.height);

        return m_cache.Add(name, GLTexture::CreateFromImage(image, filtering, encoding), lifetime);
    }

    std::optional<TextureHandle> TextureStore::LoadIfExists(const std::string& path, Core::AssetLifetime lifetime,
                                                            TextureEncoding encoding, TextureFiltering filtering)
    {
        // A path loaded before is there whether its file exists or not (a fallback), and is returned like by Load().
        if (m_cache.Find(path).has_value())
            return Load(path, lifetime, encoding, filtering);

        // The error_code overload does not throw: a path that cannot be checked counts as missing.
        std::error_code error;
        if (!std::filesystem::exists(m_assetsDirectory / path, error))
            return std::nullopt;

        return Load(path, lifetime, encoding, filtering);
    }

    void TextureStore::ExtendLifetime(TextureHandle handle, Core::AssetLifetime lifetime)
    {
        m_cache.ExtendLifetime(handle, lifetime);
    }

    void TextureStore::RemoveAll(Core::AssetLifetime lifetime)
    {
        // A removed path may be loaded again later, and its file may exist by then: it is no longer a fallback.
        for (const std::string& path : m_cache.RemoveAll(lifetime))
            m_fallbackPaths.erase(path);
    }

    const GLTexture& TextureStore::Get(TextureHandle handle) const
    {
        const GLTexture* texture = m_cache.Get(handle);
        if (texture == nullptr)
            return m_fallbackTexture;

        return *texture;
    }

    const GLTexture& TextureStore::Get(TextureHandle handle, BuiltInTexture missing) const
    {
        if (const GLTexture* texture = m_cache.Get(handle); texture != nullptr)
            return *texture;

        return Get(m_builtInTextures[static_cast<std::size_t>(missing)]);
    }

    std::size_t TextureStore::GetCount() const noexcept
    {
        return m_cache.GetCount();
    }

    const std::string* TextureStore::GetPath(TextureHandle handle) const
    {
        return m_cache.GetPath(handle);
    }
}
