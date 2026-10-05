#pragma once

#include <cstdint>
#include <string_view>

namespace Abomination::Renderer
{
    // A framebuffer: a picture in video memory that OpenGL draws into instead of the screen. This one is made for the 3D
    // scene and has two textures (its "attachments"):
    //   - color, GL_RGBA16F: four 16-bit floating-point numbers per pixel, so a pixel brighter than 1.0 (a flame five
    //     times brighter than a white wall) keeps its value instead of being cut to 1.0 like on the 8-bit screen
    //     (HDR, high dynamic range);
    //   - depth, GL_DEPTH_COMPONENT32F: how far the closest surface of every pixel is, for the depth test, like the depth
    //     buffer of the screen. A texture, not a hidden buffer, so later passes can read it (SSAO, soft particles).
    // Both are deleted in the destructor. Move-only.
    class GLFramebuffer
    {
    public:
        // A framebuffer of width x height pixels (both greater than 0). debugName is shown by RenderDoc.
        [[nodiscard]] static GLFramebuffer Create(int width, int height, std::string_view debugName);

        GLFramebuffer(const GLFramebuffer&) = delete;
        GLFramebuffer& operator=(const GLFramebuffer&) = delete;

        GLFramebuffer(GLFramebuffer&& other) noexcept;
        GLFramebuffer& operator=(GLFramebuffer&& other) noexcept;

        ~GLFramebuffer();

        // Draws go into this framebuffer from now on, over its whole size (the viewport is set to it).
        void Bind() const;

        // Connects the color texture to a texture unit, for a shader that reads the picture (see layout(binding = N)).
        void BindColorTexture(std::uint32_t unit) const;

        [[nodiscard]] int GetWidth() const noexcept;
        [[nodiscard]] int GetHeight() const noexcept;

    private:
        GLFramebuffer(std::uint32_t framebufferID, std::uint32_t colorTextureID, std::uint32_t depthTextureID, int width,
                      int height) noexcept;

        // Deletes the OpenGL objects (nothing for 0, a moved-from object).
        void Release() noexcept;

        std::uint32_t m_framebufferID = 0;
        std::uint32_t m_colorTextureID = 0;
        std::uint32_t m_depthTextureID = 0;
        int m_width = 0;
        int m_height = 0;
    };
}
