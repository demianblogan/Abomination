#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>

namespace Abomination::Core
{
    struct Image;
}

namespace Abomination::Renderer
{
    // A 2D texture in video memory: a picture the fragment shader can read colors from.
    // It stores the picture itself and all its mipmap levels (smaller copies for distant surfaces), and uses
    // pixel-crisp filtering for the retro look. The texture is deleted in the destructor. Move-only.
    class GLTexture
    {
    public:
        // Uploads the pixels of the image to the GPU and generates the mipmap levels.
        [[nodiscard]] static GLTexture CreateFromImage(const Core::Image& image);

        // Loads an image file and calls CreateFromImage(). Returns an error if the file cannot be loaded.
        [[nodiscard]] static std::expected<GLTexture, std::string> CreateFromFile(const std::filesystem::path& path);

        GLTexture(const GLTexture&) = delete;
        GLTexture& operator=(const GLTexture&) = delete;

        GLTexture(GLTexture&& other) noexcept;
        GLTexture& operator=(GLTexture&& other) noexcept;

        ~GLTexture();

        // Connects the texture to a texture unit: a numbered slot a sampler in the shader reads from.
        // unit must match layout(binding = N) of the sampler2D in the shader.
        void Bind(std::uint32_t unit) const;

        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

        // Bytes of video memory the texture takes: all its mipmap levels, 4 bytes per texel.
        [[nodiscard]] std::size_t GetVideoMemorySize() const noexcept;

        // Bytes of video memory a texture of this size takes with all its mipmap levels. Needs no OpenGL, so it can be
        // tested and used for estimates before a texture is created.
        [[nodiscard]] static std::size_t CalculateVideoMemorySize(int width, int height) noexcept;

    private:
        GLTexture(std::uint32_t textureID, int width, int height) noexcept;

        // 0 means "no texture" (a moved-from object).
        std::uint32_t m_textureID = 0;
        int m_width = 0;
        int m_height = 0;
    };
}
