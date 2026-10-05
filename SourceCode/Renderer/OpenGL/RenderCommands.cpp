#include "Renderer/OpenGL/RenderCommands.h"

#include <glad/gl.h>

#include <cstddef>

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

    Core::Image ReadFramePixels(int widthInPixels, int heightInPixels)
    {
        Core::Image image{.width = widthInPixels, .height = heightInPixels};
        image.pixels.resize(static_cast<std::size_t>(widthInPixels) * static_cast<std::size_t>(heightInPixels) *
                            Core::ImageChannelCount);

        // Rows packed one after another without padding (OpenGL would round every row up to 4 bytes; with 4 bytes per
        // pixel it never has to, but the image must not depend on that). OpenGL reads from the bottom row up, the order of
        // Core::Image.
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, widthInPixels, heightInPixels, GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data());

        for (std::size_t alpha = 3; alpha < image.pixels.size(); alpha += Core::ImageChannelCount)
            image.pixels[alpha] = 255;

        return image;
    }
}
