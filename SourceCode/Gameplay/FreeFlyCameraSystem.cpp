#include "Gameplay/FreeFlyCameraSystem.h"

#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/MouseLook.h"
#include "Physics/CharacterMovement.h"
#include "World/CollisionDebug.h"

#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
    void UpdateFreeFlyCameraLook(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                                 const Input::Mouse& mouse)
    {
        if (state.controlMode != ControlMode::FreeFlyCamera)
            return;

        Core::Transform& transform = registry.get<Core::Transform>(state.freeFlyCamera);
        state.freeFlyCameraController.UpdateRotation(registry.get<LookAngles>(state.freeFlyCamera), transform, actions, mouse);

        // The rotation from the mouse is already up to date in this frame, so it must not be interpolated between ticks:
        // drawing a rotation between the last two ticks would make the view lag behind the mouse. Setting the previous
        // rotation to the current one makes the interpolation give exactly the current rotation, while the position
        // (changed in ticks) is still interpolated.
        registry.get<Core::PreviousTransform>(state.freeFlyCamera).value.rotation = transform.rotation;
    }

    void UpdateFreeFlyCamera(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                             std::span<const World::CollisionBrush> brushes, bool doesCollide, float tickDuration)
    {
        if (state.controlMode != ControlMode::FreeFlyCamera)
            return;

        Core::Transform& transform = registry.get<Core::Transform>(state.freeFlyCamera);
        const glm::vec3 positionBefore = transform.position;
        state.freeFlyCameraController.UpdateMovement(transform, actions, tickDuration);

        // A colliding camera makes the same move again, but through the level: it slides along what it hits, like the
        // player (Physics::SlideMove), instead of flying through. The controller has already moved the transform, so
        // the move is turned into a velocity for this tick. A camera that starts inside a brush (the tool was switched
        // on in a wall) keeps the free move, so it can get out.
        glm::dvec3 position(positionBefore);
        if (doesCollide && !Physics::IsInSolid(brushes, position, World::CameraHalfExtents))
        {
            glm::vec3 velocity = (transform.position - positionBefore) / tickDuration;
            Physics::SlideMove(brushes, position, velocity, World::CameraHalfExtents, tickDuration);
            transform.position = glm::vec3(position);
        }
    }
}
