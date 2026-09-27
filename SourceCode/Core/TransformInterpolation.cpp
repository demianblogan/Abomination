#include "Core/TransformInterpolation.h"

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Abomination::Core
{
    void EnableInterpolation(entt::registry& registry, entt::entity entity)
    {
        // emplace_or_replace: works whether or not the entity already had the component.
        registry.emplace_or_replace<PreviousTransform>(entity, registry.get<Transform>(entity));
    }

    void StorePreviousTransforms(entt::registry& registry)
    {
        registry.view<const Transform, PreviousTransform>().each([](const Transform& transform, PreviousTransform& previous)
        {
            previous.value = transform;
        });
    }

    Transform InterpolateTransform(const Transform& previous, const Transform& current, float alpha)
    {
        // glm::mix(a, b, t) = a + (b - a) * t: a point on the straight line from a to b.
        // A straight line does not work for rotations: blending the 4 numbers of two quaternions would turn unevenly and
        // give a quaternion of the wrong length. glm::slerp ("spherical linear interpolation") turns along the shortest
        // arc between the two rotations at a constant speed.
        return Transform{
            .position = glm::mix(previous.position, current.position, alpha),
            .rotation = glm::slerp(previous.rotation, current.rotation, alpha),
            .scale = glm::mix(previous.scale, current.scale, alpha),
        };
    }
}
