#pragma once

#include "Renderer/OpenGL/GLFramebuffer.h"
#include "Renderer/OpenGL/GLVertexArray.h"

#include <glm/vec4.hpp>

#include <optional>

namespace Abomination::Renderer
{
    class GLShaderProgram;

    // Where the 3D scene is drawn before it reaches the screen: an HDR framebuffer of the size of the window (see
    // GLFramebuffer), holding linear values that may be brighter than 1. A frame:
    //
    //   Begin()    the world, the effects, the debug lines and the weapon are drawn into the framebuffer
    //   Present()  one triangle over the whole screen reads every pixel of it and writes its color onto the screen,
    //              converted into what the screen expects (the Present shader: linear -> sRGB)
    //
    // after which the game interface and the debug overlay are drawn on the screen as before, in sRGB. Requires a current
    // OpenGL context. Move-only.
    class SceneFramebuffer
    {
    public:
        // Draws go into the framebuffer from now on. It is made at the first frame and made again when the size of the
        // window changes (both greater than 0). clearColor: the background, in linear values; the depth is cleared too.
        void Begin(int widthInPixels, int heightInPixels, const glm::vec4& clearColor);

        // Writes the scene onto the screen through program (Present) and leaves the screen bound for what is drawn
        // over the scene. Only after Begin() in the same frame.
        void Present(const GLShaderProgram& program) const;

    private:
        std::optional<GLFramebuffer> m_framebuffer;

        // The Present triangle needs no vertex data (its corners come from gl_VertexID), but OpenGL Core draws nothing
        // without a vertex array bound, so an empty one is kept for it.
        GLVertexArray m_emptyVertexArray;
    };
}
