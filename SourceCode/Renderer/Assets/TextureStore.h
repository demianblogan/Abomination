#pragma once

#include "Core/Assets/AssetCache.h"
#include "Core/Assets/AssetHandle.h"
#include "Core/Assets/AssetLifetime.h"
#include "Core/Files/Image.h"
#include "Renderer/OpenGL/GLTexture.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>

namespace Abomination::Renderer
{
    using TextureHandle = Core::AssetHandle<GLTexture>;

    // Textures of one texel every store has, for the maps a material does not have (see Renderer::Material): the shader
    // reads every map the same way, and a missing map changes nothing.
    enum class BuiltInTexture
    {
        // 1.0 in every channel: a factor times white is the factor (roughness and metalness without a map).
        White,

        // 0.0: no light given off (emission without a map).
        Black,

        // (0.5, 0.5, 1.0): the normal (0, 0, 1) of a flat surface, straight out of it (a normal map without bumps).
        FlatNormal,
    };

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
        // encoding: SRGB for colors, Raw for maps that hold data (see TextureEncoding). A path is loaded with one encoding:
        // the first one it is asked for.
        [[nodiscard]] TextureHandle Load(const std::string& path, Core::AssetLifetime lifetime,
                                         TextureEncoding encoding = TextureEncoding::SRGB);

        // Like Load, but only if the file exists: nothing (and no warning) for a map a material does not have. A file that
        // exists but is broken still gives the checkerboard.
        [[nodiscard]] std::optional<TextureHandle> LoadIfExists(const std::string& path, Core::AssetLifetime lifetime,
                                                                TextureEncoding encoding);

        // Stores a texture made of an image already in memory (a texture inside a model file) under name, named like a
        // path: "Models/Weapons/Shotgun.glb#image0". If a texture with this name is loaded, it is returned as it is. An
        // empty image (one that could not be decoded) gives the checkerboard, like a missing file.
        [[nodiscard]] TextureHandle Add(const std::string& name, const Core::Image& image, Core::AssetLifetime lifetime,
                                        TextureEncoding encoding = TextureEncoding::SRGB);

        // Moves the texture to the longer of its lifetime and the given one (see AssetCache::ExtendLifetime).
        void ExtendLifetime(TextureHandle handle, Core::AssetLifetime lifetime);

        // Removes every texture of the lifetime group from video memory; their handles become invalid.
        void RemoveAll(Core::AssetLifetime lifetime);

        // The texture of the handle. An invalid handle (default-constructed or of a removed texture) gives the
        // checkerboard, so drawing code never has to check for nullptr.
        [[nodiscard]] const GLTexture& Get(TextureHandle handle) const;

        // The texture of the handle, or the built-in texture for an invalid handle (a map the material does not have).
        [[nodiscard]] const GLTexture& Get(TextureHandle handle, BuiltInTexture missing) const;

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

        // The built-in textures, in the order of BuiltInTexture. Also in the cache ("Built-in/White", ...), so the Assets
        // window lists them; kept here as well, so Get() needs no lookup.
        std::array<TextureHandle, 3> m_builtInTextures;
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
