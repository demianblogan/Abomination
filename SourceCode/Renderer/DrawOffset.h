#pragma once

#include <glm/vec3.hpp>

namespace Abomination::Renderer
{
    // Component: draws the entity moved by an offset from its Core::Transform, which stays where the simulation put it.
    // A character that steps up a stair is moved up at once by the physics; the offset keeps its model a little lower
    // and lets it glide up (see Gameplay::StepSmoothing). Like the transform, the offset has a value now and one tick
    // earlier, and is drawn between the two.
    struct DrawOffset
    {
        glm::vec3 offset{0.0f};
        glm::vec3 previousOffset{0.0f};

        // A tilt of the model around its own right axis, in radians: positive raises its front (a dog on a stair, its
        // front paws a step higher than its hind paws; see Gameplay::GroundFit).
        float pitch = 0.0f;
        float previousPitch = 0.0f;
    };
}
