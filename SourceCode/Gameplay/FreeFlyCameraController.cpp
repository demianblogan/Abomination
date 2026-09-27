#include "Gameplay/FreeFlyCameraController.h"

#include "Core/Name.h"
#include "Core/Transform.h"
#include "Core/TransformInterpolation.h"
#include "Input/ActionStates.h"
#include "Input/Mouse.h"
#include "Renderer/CameraLens.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

namespace Abomination::Gameplay
{
    using Input::Action;

    namespace
    {
        // The vertical axis of the world: the yaw turns around it, and E and Q fly along it.
        constexpr glm::vec3 WorldUp{0.0f, 1.0f, 0.0f};

        // The camera's own directions (in its local coordinates): it looks along -Z, its right side is +X. Turned by the
        // camera's rotation (rotation * direction) they give where the camera looks and where its right side is in the world.
        constexpr glm::vec3 LocalForward{0.0f, 0.0f, -1.0f};
        constexpr glm::vec3 LocalRight{1.0f, 0.0f, 0.0f};

        // 1.0 while the action is active, 0.0 otherwise: lets opposite actions cancel each other out by subtraction.
        float GetActionValue(const Input::ActionStates& actions, Action action) noexcept
        {
            return actions.IsActionActive(action) ? 1.0f : 0.0f;
        }
    }

    entt::entity SpawnFreeFlyCamera(entt::registry& registry, glm::vec3 position, float yaw)
    {
        const entt::entity camera = registry.create();
        registry.emplace<Core::Name>(camera, "Camera");
        const Core::Transform transform{.position = position, .rotation = CalculateCameraRotation(yaw, 0.0f)};
        registry.emplace<Core::Transform>(camera, transform);
        registry.emplace<Renderer::CameraLens>(camera);
        registry.emplace<FreeFlyCamera>(camera, FreeFlyCamera{.yaw = yaw});
        Core::EnableInterpolation(registry, camera);

        return camera;
    }

    FreeFlyCameraController::FreeFlyCameraController(const FreeFlyCameraSettings& settings) noexcept
        : m_settings(settings)
    {}

    void FreeFlyCameraController::UpdateRotation(FreeFlyCamera& camera, Core::Transform& transform,
                                                 const Input::ActionStates& actions, const Input::Mouse& mouse) const
    {
        // Only while LookAroundMode is active, and not in the frame it starts: switching the mouse into relative mode
        // can produce one big jump of movement in that frame, which would snap the camera.
        const bool isLookingAround =
            actions.IsActionActive(Action::LookAroundMode) && !actions.WasActionStarted(Action::LookAroundMode);
        if (!isLookingAround)
            return;

        TurnByMouse(camera.yaw, camera.pitch, mouse.GetMovement(), m_settings.mouseSensitivity);

        transform.rotation = CalculateCameraRotation(camera.yaw, camera.pitch);
    }

    void FreeFlyCameraController::UpdateMovement(Core::Transform& transform, const Input::ActionStates& actions,
                                                 float deltaTime) const
    {
        // 1. Direction of movement. Every pair of opposite actions gives the input along its axis: -1, 0 or +1
        //    (W alone: +1 forward, S alone: -1, both or none: 0, so the camera stays in place).
        const float forwardInput =
            GetActionValue(actions, Action::MoveForward) - GetActionValue(actions, Action::MoveBackward);
        const float rightInput = GetActionValue(actions, Action::MoveRight) - GetActionValue(actions, Action::MoveLeft);
        const float upInput = GetActionValue(actions, Action::MoveUp) - GetActionValue(actions, Action::MoveDown);

        // The camera's own directions in the world: the rotation applied to its local directions (quaternion * vector
        // turns the vector). Forward includes looking up or down; right stays horizontal, because the camera never rolls
        // (the pitch turns around the local X axis, which leaves the local right direction unchanged).
        // Up is the world vertical axis, not the camera's: E and Q fly straight up and down wherever the camera looks.
        const glm::vec3 forward = transform.rotation * LocalForward;
        const glm::vec3 right = transform.rotation * LocalRight;

        glm::vec3 direction = forward * forwardInput + right * rightInput + WorldUp * upInput;
        if (direction == glm::vec3(0.0f))
            return;

        // Without normalizing, W + D together would give a vector of length about 1.41: diagonal flying would be
        // 41% faster than straight flying. Normalizing makes the length 1 in every direction.
        direction = glm::normalize(direction);

        // 2. Distance for this tick: speed (meters per second) times the tick duration (seconds) gives meters.
        float speed = m_settings.moveSpeed;
        if (actions.IsActionActive(Action::MoveFaster))
            speed *= m_settings.fastMoveMultiplier;

        transform.position += direction * speed * deltaTime;
    }
}
