#pragma once

#include "Core/Assets/AssetCache.h"
#include "Core/Assets/AssetHandle.h"
#include "Renderer/OpenGL/GLShaderProgram.h"

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace Abomination::Renderer
{
    using ShaderHandle = Core::AssetHandle<GLShaderProgram>;

    // Loads shader programs and keeps every program exactly once. A program is named by the path of its two files
    // without the extension: "Shaders/TexturedMesh" is Shaders/TexturedMesh.vert + Shaders/TexturedMesh.frag.
    //
    // A missing file or a compilation error does not stop the game: the program is replaced by a fallback program that
    // draws everything in plain magenta, and the error (with the compiler log) is logged. The fallback is stored under
    // the name of the broken program, so it is compiled and reported only once.
    //
    // The fallback program expects what every program of the game provides: the vertex position at location 0 and
    // the model, view and projection matrices at uniform locations 0, 1 and 2.
    //
    // Shader programs are always global (see Core::AssetLifetime): there are a handful of them, and every level uses them.
    //
    // Requires a current OpenGL context. Move-only.
    class ShaderStore
    {
    public:
        // assetsDirectory: the folder all shader names are relative to. Fails only if the built-in fallback program
        // cannot be compiled, which means the OpenGL driver is broken.
        [[nodiscard]] static std::expected<ShaderStore, std::string> Create(std::filesystem::path assetsDirectory);

        // Returns the program with this name, loading it on the first call. The same name always gives the same handle.
        [[nodiscard]] ShaderHandle Load(const std::string& name);

        // The program of the handle. An invalid handle gives the fallback program.
        [[nodiscard]] const GLShaderProgram& Get(ShaderHandle handle) const;

        // Calls visitor(name, isFallback) for every loaded program; isFallback is true for a program replaced by the
        // magenta fallback. For the Assets window of the debug overlay.
        template <typename Visitor>
        void VisitPrograms(Visitor&& visitor) const;

        // The name the program was loaded with, or nullptr for an invalid handle. For the entity inspector.
        [[nodiscard]] const std::string* GetName(ShaderHandle handle) const;

        [[nodiscard]] std::size_t GetCount() const noexcept;

    private:
        ShaderStore(std::filesystem::path assetsDirectory, GLShaderProgram fallbackProgram) noexcept;

        std::filesystem::path m_assetsDirectory;
        Core::AssetCache<GLShaderProgram> m_cache;

        // Names of programs that could not be loaded or compiled and hold the fallback instead.
        std::unordered_set<std::string> m_fallbackNames;

        // Returned by Get() for invalid handles.
        GLShaderProgram m_fallbackProgram;
    };

    template <typename Visitor>
    void ShaderStore::VisitPrograms(Visitor&& visitor) const
    {
        m_cache.VisitAssets([&](const std::string& name, const GLShaderProgram&, Core::AssetLifetime)
        {
            visitor(name, m_fallbackNames.contains(name));
        });
    }
}
