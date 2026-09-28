#pragma once

#include <glm/vec4.hpp>

// Basic commands that prepare the frame for drawing. They will become part of the high-level renderer API later.
namespace Abomination::Renderer
{
    // Sets the rectangle of the window OpenGL draws into, in pixels, starting at the bottom-left corner.
    // Must match the window size, otherwise the picture is stretched or cut off after the window is resized.
    void SetViewport(int widthInPixels, int heightInPixels);

    // Fills the whole frame with one color (red, green, blue, alpha; each from 0 to 1) and resets the depth buffer.
    void ClearFrame(const glm::vec4& color);
}
