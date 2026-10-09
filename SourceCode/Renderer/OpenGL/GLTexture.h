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
    // How a texture looks when it is drawn larger or smaller than its own size.
    enum class TextureFiltering
    {
        // Crisp square texels close up, the retro look of the level and the models (see CreateFromImage).
        Pixelated,

        // Texels blended smoothly at every size: text and interface icons, whose edges must stay soft, not stepped; and the
        // maps that light a surface (normals, roughness, heights), while its color stays crisp: a normal jumping from texel
        // to texel makes the highlights flicker when the camera moves.
        Smooth,
    };

    // What the numbers of a color texture mean, and so whether the GPU converts them when a shader reads them (see
    // Documentation/ARCHITECTURE.md, section 6, linear lighting).
    enum class TextureEncoding
    {
        // Colors as image files store them, in sRGB: the GPU turns them into linear values (proportional to the amount of
        // light) when a shader reads them, for free (GL_SRGB8_ALPHA8). The textures of the world, the models and the
        // effects, which are lit and blended in linear values.
        SRGB,

        // The numbers as they are, nothing converted (GL_RGBA8): maps that hold data, not colors (normals, roughness and
        // metalness, heights), and the game interface, which is drawn and blended in sRGB, the way its designer saw it.
        Raw,
    };

    // A 2D texture in video memory: a picture the fragment shader can read colors from.
    // It stores the picture itself and all its mipmap levels (smaller copies for distant surfaces), and uses
    // pixel-crisp filtering for the retro look (or smooth filtering, see TextureFiltering). The texture is deleted in the
    // destructor. Move-only.
    class GLTexture
    {
    public:
        // Uploads the pixels of the image to the GPU and generates the mipmap levels.
        [[nodiscard]] static GLTexture CreateFromImage(const Core::Image& image,
                                                       TextureFiltering filtering = TextureFiltering::Pixelated,
                                                       TextureEncoding encoding = TextureEncoding::SRGB);

        // Loads an image file and calls CreateFromImage(). Returns an error if the file cannot be loaded.
        [[nodiscard]] static std::expected<GLTexture, std::string> CreateFromFile(
            const std::filesystem::path& path, TextureFiltering filtering = TextureFiltering::Pixelated,
            TextureEncoding encoding = TextureEncoding::SRGB);

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

        // Bytes of video memory the texture takes: all its mipmap levels, 4 bytes per texel. Only calls
        // CalculateVideoMemorySize with the size of this texture: the formula itself lives there because a GLTexture
        // cannot be created without an OpenGL context, so a method of the object could not be tested.
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
