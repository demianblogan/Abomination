#pragma once

#include "Core/Files/Image.h"

#include <glm/vec4.hpp>

// Basic commands that prepare the frame for drawing. They will become part of the high-level renderer API later.
namespace Abomination::Renderer
{
    // Sets the rectangle of the window OpenGL draws into, in pixels, starting at the bottom-left corner.
    // Must match the window size, otherwise the picture is stretched or cut off after the window is resized.
    void SetViewport(int widthInPixels, int heightInPixels);

    // Fills the whole frame with one color (red, green, blue, alpha; each from 0 to 1) and resets the depth buffer.
    void ClearFrame(const glm::vec4& color);

    // Resets only the depth buffer: what is drawn next is not hidden by anything drawn before, but the colors stay.
    // The weapon in the hands is drawn after it (see DrawWeaponViewModel), so it never disappears into a wall.
    void ClearDepth();

    // The picture drawn so far in the frame on the screen (the back buffer, before it is swapped), as an image of the size
    // given. Alpha is set to 255: the alpha of the screen means nothing. Slow (the CPU waits for the GPU to finish the
    // frame): for a screenshot, not for every frame.
    [[nodiscard]] Core::Image ReadFramePixels(int widthInPixels, int heightInPixels);
}
