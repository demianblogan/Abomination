#pragma once

#include <glm/trigonometric.hpp>

namespace Abomination::Renderer
{
    // Component: the lens of a camera entity, how wide it sees and how near and far. Where the camera stands and where
    // it looks is its Core::Transform: the camera looks along its local -Z axis (OpenGL's "forward"), its local +Y is
    // the top of the screen. Together they give the View the renderer draws through (see View.h).
    struct CameraLens
    {
        // Vertical field of view in radians: how wide the camera sees from the bottom edge of the screen to the top.
        // The horizontal one follows from the aspect ratio of the window.
        float verticalFOV = glm::radians(60.0f);

        // Nothing closer than the near plane or farther than the far plane (in meters) is drawn. The depth buffer has
        // limited precision between them, so the near plane should not be closer than needed.
        float nearPlane = 0.1f;
        float farPlane = 100.0f;
    };
}
