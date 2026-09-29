#pragma once

#include "Core/Scene/Transform.h"
#include "Renderer/Camera/CameraLens.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Abomination::Renderer
{
    // Everything the renderer needs to know about the camera of a frame: two matrices and the position.
    // The renderer does not know which entity is the camera, how it moves or whether it is interpolated: the application
    // calculates the View once per frame and passes it in.
    struct View
    {
        // Moves the world so that the camera ends up at the origin looking along -Z.
        glm::mat4 viewMatrix{1.0f};

        // Applies perspective: distant points move closer to the center, so distant things look smaller.
        glm::mat4 projectionMatrix{1.0f};

        // Where the camera is in the world (for lighting and sorting later).
        glm::vec3 position{0.0f};
    };

    // The View of a camera standing at cameraTransform with the given lens. aspectRatio is the width of the drawable
    // area divided by its height (must be above 0). The scale of the transform is ignored: a camera has no size.
    [[nodiscard]] View CalculateView(const Core::Transform& cameraTransform, const CameraLens& lens, float aspectRatio);
}
