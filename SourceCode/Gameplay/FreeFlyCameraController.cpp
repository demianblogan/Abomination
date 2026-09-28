#include "Gameplay/FreeFlyCameraController.h"

#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Input/ActionStates.h"
#include "Input/Mouse.h"
#include "Renderer/Camera/CameraLens.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

namespace Abomination::Gameplay
{
    using Input::Action;

    entt::entity SpawnFreeFlyCamera(entt::registry& registry, glm::vec3 position, float yaw)
    {
        const entt::entity camera = registry.create();
        registry.emplace<Core::Name>(camera, "Camera");
        const LookAngles look{.yaw = yaw};
        const Core::Transform transform{.position = position, .rotation = CalculateCameraRotation(look)};
        registry.emplace<Core::Transform>(camera, transform);
        registry.emplace<Renderer::CameraLens>(camera);
        registry.emplace<LookAngles>(camera, look);
        Core::EnableInterpolation(registry, camera);

        return camera;
    }

    FreeFlyCameraController::FreeFlyCameraController(const FreeFlyCameraSettings& settings) noexcept
        : m_settings(settings)
    {}

    void FreeFlyCameraController::UpdateRotation(LookAngles& look, Core::Transform& transform,
                                                 const Input::ActionStates& actions, const Input::Mouse& mouse) const
    {
        // Only while LookAroundMode is active, and not in the frame it starts: switching the mouse into relative mode
        // can produce one big jump of movement in that frame, which would snap the camera.
        const bool isLookingAround =
            actions.IsActionActive(Action::LookAroundMode) && !actions.WasActionStarted(Action::LookAroundMode);
        if (!isLookingAround)
            return;

        TurnByMouse(look, mouse.GetMovement(), m_settings.mouseSensitivity);

        transform.rotation = CalculateCameraRotation(look);
    }

    void FreeFlyCameraController::UpdateMovement(Core::Transform& transform, const Input::ActionStates& actions,
                                                 float deltaTime) const
    {
        // 1. Direction of movement. Every pair of opposite actions gives the input along its axis: -1, 0 or +1
        //    (W alone: +1 forward, S alone: -1, both or none: 0, so the camera stays in place).
        const float forwardInput = actions.GetAxis(Action::MoveForward, Action::MoveBackward);
        const float rightInput = actions.GetAxis(Action::MoveRight, Action::MoveLeft);
        const float upInput = actions.GetAxis(Action::MoveUp, Action::MoveDown);

        // The camera's own directions in the world: the rotation applied to its local directions (quaternion * vector
        // turns the vector). Forward includes looking up or down; right stays horizontal, because the camera never rolls
        // (the pitch turns around the local X axis, which leaves the local right direction unchanged).
        // Up is the world vertical axis, not the camera's: E and Q fly straight up and down wherever the camera looks.
        const glm::vec3 forward = transform.rotation * Core::LocalForward;
        const glm::vec3 right = transform.rotation * Core::LocalRight;

        glm::vec3 direction = forward * forwardInput + right * rightInput + Core::WorldUp * upInput;
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
