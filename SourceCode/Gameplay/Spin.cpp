#include "Gameplay/Spin.h"

#include "Core/Scene/Transform.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Abomination::Gameplay
{
    void UpdateSpinningEntities(entt::registry& registry, float deltaTime)
    {
        registry.view<Core::Transform, const Spin>().each([deltaTime](Core::Transform& transform, const Spin& spin)
        {
            // A zero axis has no direction to turn around (normalizing it would divide by zero).
            if (spin.axis == glm::vec3(0.0f))
                return;

            // glm::angleAxis builds the quaternion "turn by this angle around this axis" (the axis must be normalized).
            // Multiplying quaternions combines rotations; the new turn is on the LEFT, so it is applied after the current
            // rotation and around the world axis, not around the already turned axes of the entity.
            // A rotation quaternion must have length 1. Each multiplication adds a tiny float error, and after thousands
            // of ticks the length would drift and the object would start to stretch; normalizing keeps it at 1.
            const glm::quat step = glm::angleAxis(spin.speed * deltaTime, glm::normalize(spin.axis));
            transform.rotation = glm::normalize(step * transform.rotation);
        });
    }
}
