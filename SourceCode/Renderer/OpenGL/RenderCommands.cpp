#include "Renderer/OpenGL/RenderCommands.h"

#include <glad/gl.h>

namespace Abomination::Renderer
{
    void SetViewport(int widthInPixels, int heightInPixels)
    {
        glViewport(0, 0, widthInPixels, heightInPixels);
    }

    void ClearFrame(const glm::vec4& color)
    {
        // glClearColor only remembers the color in the context state; glClear fills the back buffer with it.
        // The depth buffer is cleared too: every pixel gets the largest depth (1.0, "infinitely far"), so the first
        // surface drawn into a pixel always passes the depth test. Without this, last frame's depth would remain.
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void ClearDepth()
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}
