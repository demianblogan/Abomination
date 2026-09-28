#pragma once

#include "Core/Assets/AssetCache.h"
#include "Core/Assets/AssetHandle.h"
#include "Core/Assets/AssetLifetime.h"
#include "Renderer/OpenGL/GLTexture.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Abomination::Renderer
{
    using TextureHandle = Core::AssetHandle<GLTexture>;

    // Loads textures from image files and keeps every texture exactly once, however many objects use it.
    //
    // A missing or broken file does not stop the game: the texture is replaced by a magenta and black checkerboard
    // that cannot be overlooked (the "missing texture" of Source, Unity and other engines), and a warning is logged.
    // The checkerboard is stored under the path of the missing file, so the file is looked for and reported only once.
    //
    // Requires a current OpenGL context: textures are created in video memory. Move-only.
    class TextureStore
    {
    public:
        // assetsDirectory: the folder all texture paths are relative to.
        explicit TextureStore(std::filesystem::path assetsDirectory);

        // Returns the texture loaded from path, loading it on the first call. path is relative to the assets directory
        // and uses forward slashes: "Textures/Episode1/Crate_Rotten.png". The same path always gives the same handle.
        // The texture stays loaded for the lifetime (the longer one if it is asked for again with another lifetime).
        [[nodiscard]] TextureHandle Load(const std::string& path, Core::AssetLifetime lifetime);

        // Removes every texture of the lifetime group from video memory; their handles become invalid.
        void RemoveAll(Core::AssetLifetime lifetime);

        // The texture of the handle. An invalid handle (default-constructed or of a removed texture) gives the
        // checkerboard, so drawing code never has to check for nullptr.
        [[nodiscard]] const GLTexture& Get(TextureHandle handle) const;

        // Calls visitor(path, texture, isFallback, lifetime) for every loaded texture; isFallback is true for a missing or
        // broken file replaced by the checkerboard. For the Assets window of the debug overlay.
        template <typename Visitor>
        void VisitTextures(Visitor&& visitor) const;

        // The path the texture was loaded from, or nullptr for an invalid handle. For the entity inspector.
        [[nodiscard]] const std::string* GetPath(TextureHandle handle) const;

        [[nodiscard]] std::size_t GetCount() const noexcept;

    private:
        std::filesystem::path m_assetsDirectory;
        Core::AssetCache<GLTexture> m_cache;

        // Paths whose file could not be loaded and which hold a checkerboard instead.
        std::unordered_set<std::string> m_fallbackPaths;

        // Returned by Get() for invalid handles.
        GLTexture m_fallbackTexture;
    };

    template <typename Visitor>
    void TextureStore::VisitTextures(Visitor&& visitor) const
    {
        m_cache.VisitAssets([&](const std::string& path, const GLTexture& texture, Core::AssetLifetime lifetime)
        {
            visitor(path, texture, m_fallbackPaths.contains(path), lifetime);
        });
    }
}
