#include "Renderer/Camera/View.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cassert>

namespace Abomination::Renderer
{
    View CalculateView(const Core::Transform& cameraTransform, const CameraLens& lens, float aspectRatio)
    {
        // A minimized window has a height of 0; the caller must not ask for a View then (glm::perspective divides by it).
        assert(aspectRatio > 0.0f);

        // The camera's own matrix (without scale) would place a "camera object" in the world: rotate, then move to its
        // position. The view matrix must do the opposite to the whole world, so that the camera ends up at the origin
        // looking along -Z: first move everything by -position, then turn it back by the inverse rotation.
        // For a rotation quaternion the inverse is the conjugate (the same axis, the opposite angle), which is cheaper.
        const glm::mat4 inverseRotation = glm::mat4_cast(glm::conjugate(cameraTransform.rotation));
        const glm::mat4 inverseTranslation = glm::translate(glm::mat4(1.0f), -cameraTransform.position);

        return View{
            .viewMatrix = inverseRotation * inverseTranslation,
            .projectionMatrix = glm::perspective(lens.verticalFOV, aspectRatio, lens.nearPlane, lens.farPlane),
            .position = cameraTransform.position,
        };
    }
}
