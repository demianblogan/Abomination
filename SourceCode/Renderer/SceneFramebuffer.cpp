#include "Renderer/SceneFramebuffer.h"

#include "Core/Profiling/ProfileZone.h"
#include "Renderer/OpenGL/GLShaderProgram.h"
#include "Renderer/OpenGL/GPUProfileZone.h"
#include "Renderer/OpenGL/RenderCommands.h"
#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>

#include <cassert>

namespace Abomination::Renderer
{
    void SceneFramebuffer::Begin(int widthInPixels, int heightInPixels, const glm::vec4& clearColor)
    {
        assert(widthInPixels > 0 && heightInPixels > 0);

        // A framebuffer has a fixed size: a resized window needs a new one (the old one is deleted by the assignment).
        const bool isSizeChanged = m_framebuffer.has_value() && (m_framebuffer->GetWidth() != widthInPixels ||
                                                                  m_framebuffer->GetHeight() != heightInPixels);
        if (!m_framebuffer.has_value() || isSizeChanged)
            m_framebuffer = GLFramebuffer::Create(widthInPixels, heightInPixels, "Scene");

        m_framebuffer->Bind();
        ClearFrame(clearColor);
    }

    void SceneFramebuffer::Present(const GLShaderProgram& program) const
    {
        PROFILE_ZONE();
        PROFILE_GPU_ZONE("Present");

        assert(m_framebuffer.has_value());

        // Framebuffer 0 is the screen (the back buffer of the window). Every pixel of it is written, so it is not cleared.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        SetViewport(m_framebuffer->GetWidth(), m_framebuffer->GetHeight());

        // Every pixel is written as it is: nothing in front to test against, nothing to blend with.
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        program.Use();
        m_framebuffer->BindColorTexture(SceneColorTextureUnit);
        m_emptyVertexArray.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // The other passes expect the depth test on (the game interface switches it off for itself).
        glEnable(GL_DEPTH_TEST);
    }
}
