#include "Renderer/OpenGL/GLTexture.h"

#include "Core/Files/Image.h"

#include <glad/gl.h>

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <utility>

namespace Abomination::Renderer
{
    namespace
    {
        // How many mipmap levels a texture of this size has: the full picture, then half the size, a quarter, ...
        // down to 1x1. For 64x64: 64, 32, 16, 8, 4, 2, 1 - that is 7 levels.
        // std::bit_width(64) is 7: the number of bits needed to write 64 in binary (1000000).
        GLsizei CalculateMipmapLevelCount(int width, int height)
        {
            const auto largestSide = static_cast<unsigned int>(std::max(width, height));

            return static_cast<GLsizei>(std::bit_width(largestSide));
        }
    }

    GLTexture GLTexture::CreateFromImage(const Core::Image& image, TextureFiltering filtering, TextureEncoding encoding)
    {
        // A texture of 0 texels is an OpenGL error, and fewer pixels than the size promises would be read past their end.
        assert(image.width > 0 && image.height > 0);
        assert(image.pixels.size() == static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) *
                                          Core::ImageChannelCount);

        GLuint textureID = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

        // Immutable storage for all mipmap levels at once: the format and the size cannot change later.
        // Both formats store 4 channels of 8 bits each, the same layout as Core::Image. GL_SRGB8_ALPHA8 also tells the GPU
        // that red, green and blue are sRGB: it converts them into linear values when a shader reads the texture (alpha
        // is never converted), and makes the smaller mipmap levels in linear values, so they are not too dark.
        const GLenum format = encoding == TextureEncoding::SRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        const GLsizei mipmapLevelCount = CalculateMipmapLevelCount(image.width, image.height);
        glTextureStorage2D(textureID, mipmapLevelCount, format, image.width, image.height);

        // Copy the pixels into level 0 (the full-size picture). GL_RGBA + GL_UNSIGNED_BYTE describe our data in memory.
        glTextureSubImage2D(textureID, 0, 0, 0, image.width, image.height, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());

        // Fill all smaller levels by repeatedly halving the picture.
        glGenerateTextureMipmap(textureID);

        // Filtering decides the color when a texel (a pixel of the texture) does not match a screen pixel 1:1.
        //   Magnification - the texture is close and one texel covers many screen pixels.
        //     GL_NEAREST takes the nearest texel: crisp square pixels, the look of Quake and other retro games.
        //     (GL_LINEAR would blend 4 texels and make the texture blurry.)
        //   Minification - the texture is far and many texels fall into one screen pixel.
        //     GL_NEAREST_MIPMAP_LINEAR takes the nearest texel on the two mipmap levels closest to the needed size
        //     and blends between those levels. Distant surfaces stay calm instead of flickering, and still crisp.
        // Smooth filtering blends the 4 nearest texels (GL_LINEAR) on the two closest mipmap levels and between them
        // (GL_LINEAR_MIPMAP_LINEAR, "trilinear"): text stays soft-edged at any size.
        const bool isSmooth = filtering == TextureFiltering::Smooth;
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, isSmooth ? GL_LINEAR : GL_NEAREST);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, isSmooth ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR);

        // Texture coordinates outside 0-1 repeat the texture (like tiles), which walls and floors of levels need.
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        return GLTexture(textureID, image.width, image.height);
    }

    std::expected<GLTexture, std::string> GLTexture::CreateFromFile(const std::filesystem::path& path,
                                                                 TextureFiltering filtering, TextureEncoding encoding)
    {
        const std::expected<Core::Image, std::string> image = Core::LoadImageFile(path);
        if (!image.has_value())
            return std::unexpected(image.error());

        return CreateFromImage(*image, filtering, encoding);
    }

    GLTexture::GLTexture(std::uint32_t textureID, int width, int height) noexcept
        : m_textureID(textureID)
        , m_width(width)
        , m_height(height)
    {}

    GLTexture::GLTexture(GLTexture&& other) noexcept
        : m_textureID(std::exchange(other.m_textureID, 0))
        , m_width(other.m_width)
        , m_height(other.m_height)
    {}

    GLTexture& GLTexture::operator=(GLTexture&& other) noexcept
    {
        if (this != &other)
        {
            // glDelete* functions ignore the ID 0, so a moved-from object needs no special check.
            glDeleteTextures(1, &m_textureID);
            m_textureID = std::exchange(other.m_textureID, 0);
            m_width = other.m_width;
            m_height = other.m_height;
        }

        return *this;
    }

    GLTexture::~GLTexture()
    {
        glDeleteTextures(1, &m_textureID);
    }

    void GLTexture::Bind(std::uint32_t unit) const
    {
        // Direct State Access: one call instead of glActiveTexture(GL_TEXTURE0 + unit) + glBindTexture(GL_TEXTURE_2D, id).
        glBindTextureUnit(unit, m_textureID);
    }

    int GLTexture::GetWidth() const noexcept
    {
        return m_width;
    }

    int GLTexture::GetHeight() const noexcept
    {
        return m_height;
    }

    std::size_t GLTexture::GetVideoMemorySize() const noexcept
    {
        return CalculateVideoMemorySize(m_width, m_height);
    }

    std::size_t GLTexture::CalculateVideoMemorySize(int width, int height) noexcept
    {
        // Every mipmap level is half the size of the previous one (at least 1 texel); GL_RGBA8 takes 4 bytes per texel.
        // For 64x64: 64*64 + 32*32 + 16*16 + 8*8 + 4*4 + 2*2 + 1*1 = 5461 texels = 21844 bytes, about 4/3 of level 0.
        constexpr std::size_t BytesPerTexel = 4;

        std::size_t texelCount = 0;
        const int mipmapLevelCount = CalculateMipmapLevelCount(width, height);
        for (int level = 0; level < mipmapLevelCount; ++level)
        {
            // width >> level is width / 2^level: the size of this level.
            const auto levelWidth = static_cast<std::size_t>(std::max(width >> level, 1));
            const auto levelHeight = static_cast<std::size_t>(std::max(height >> level, 1));
            texelCount += levelWidth * levelHeight;
        }

        return texelCount * BytesPerTexel;
    }
}
