#include "Renderer/OpenGL/GLFramebuffer.h"

#include "Core/Logging/Log.h"

#include <glad/gl.h>

#include <cassert>
#include <string>
#include <utility>

namespace Abomination::Renderer
{
    namespace
    {
        // A texture of one level (no mipmaps: it is drawn into and read back at its own size), read texel by texel.
        GLuint CreateAttachmentTexture(GLenum format, int width, int height)
        {
            GLuint textureID = 0;
            glCreateTextures(GL_TEXTURE_2D, 1, &textureID);
            glTextureStorage2D(textureID, 1, format, width, height);

            // Read back 1:1 (texelFetch), so no blending between texels and no wrapping is needed.
            glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            return textureID;
        }
    }

    GLFramebuffer GLFramebuffer::Create(int width, int height, std::string_view debugName)
    {
        // A texture of 0 pixels is an OpenGL error (a minimized window has a height of 0: nothing is drawn then).
        assert(width > 0 && height > 0);

        const GLuint colorTextureID = CreateAttachmentTexture(GL_RGBA16F, width, height);
        const GLuint depthTextureID = CreateAttachmentTexture(GL_DEPTH_COMPONENT32F, width, height);

        // The framebuffer itself holds no pixels: it only says which textures the fragment shader output 0 and the depth
        // test write into.
        GLuint framebufferID = 0;
        glCreateFramebuffers(1, &framebufferID);
        glNamedFramebufferTexture(framebufferID, GL_COLOR_ATTACHMENT0, colorTextureID, 0);
        glNamedFramebufferTexture(framebufferID, GL_DEPTH_ATTACHMENT, depthTextureID, 0);

        // Names for RenderDoc: "Scene", "Scene color", "Scene depth".
        const std::string colorName = std::string(debugName) + " color";
        const std::string depthName = std::string(debugName) + " depth";
        glObjectLabel(GL_FRAMEBUFFER, framebufferID, static_cast<GLsizei>(debugName.size()), debugName.data());
        glObjectLabel(GL_TEXTURE, colorTextureID, static_cast<GLsizei>(colorName.size()), colorName.data());
        glObjectLabel(GL_TEXTURE, depthTextureID, static_cast<GLsizei>(depthName.size()), depthName.data());

        // Both formats must be supported by every OpenGL 4.6 driver, so an incomplete framebuffer means a bug here.
        const GLenum status = glCheckNamedFramebufferStatus(framebufferID, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            Core::Log::Write(Core::LogCategory::Renderer, Core::LogLevel::Error,
                             "Framebuffer {} is incomplete (status 0x{:x})", debugName, status);
            assert(false && "The framebuffer is incomplete");
        }

        return GLFramebuffer(framebufferID, colorTextureID, depthTextureID, width, height);
    }

    GLFramebuffer::GLFramebuffer(std::uint32_t framebufferID, std::uint32_t colorTextureID, std::uint32_t depthTextureID,
                                 int width, int height) noexcept
        : m_framebufferID(framebufferID)
        , m_colorTextureID(colorTextureID)
        , m_depthTextureID(depthTextureID)
        , m_width(width)
        , m_height(height)
    {}

    GLFramebuffer::GLFramebuffer(GLFramebuffer&& other) noexcept
        : m_framebufferID(std::exchange(other.m_framebufferID, 0))
        , m_colorTextureID(std::exchange(other.m_colorTextureID, 0))
        , m_depthTextureID(std::exchange(other.m_depthTextureID, 0))
        , m_width(other.m_width)
        , m_height(other.m_height)
    {}

    GLFramebuffer& GLFramebuffer::operator=(GLFramebuffer&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            m_framebufferID = std::exchange(other.m_framebufferID, 0);
            m_colorTextureID = std::exchange(other.m_colorTextureID, 0);
            m_depthTextureID = std::exchange(other.m_depthTextureID, 0);
            m_width = other.m_width;
            m_height = other.m_height;
        }

        return *this;
    }

    GLFramebuffer::~GLFramebuffer()
    {
        Release();
    }

    void GLFramebuffer::Release() noexcept
    {
        // glDelete* functions ignore the ID 0, so a moved-from object needs no special check.
        glDeleteFramebuffers(1, &m_framebufferID);
        glDeleteTextures(1, &m_colorTextureID);
        glDeleteTextures(1, &m_depthTextureID);
    }

    void GLFramebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferID);
        glViewport(0, 0, m_width, m_height);
    }

    void GLFramebuffer::BindColorTexture(std::uint32_t unit) const
    {
        glBindTextureUnit(unit, m_colorTextureID);
    }

    int GLFramebuffer::GetWidth() const noexcept
    {
        return m_width;
    }

    int GLFramebuffer::GetHeight() const noexcept
    {
        return m_height;
    }
}
