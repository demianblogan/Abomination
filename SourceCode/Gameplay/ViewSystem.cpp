#include "Gameplay/ViewSystem.h"

#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/MouseLook.h"
#include "Gameplay/Player.h"
#include "Renderer/Camera/CameraLens.h"

#include <glm/common.hpp>

namespace Abomination::Gameplay
{
    namespace
    {
        entt::entity GetViewEntity(const GameplayState& state)
        {
            return state.controlMode == ControlMode::Player ? state.player : state.freeFlyCamera;
        }
    }

    void ToggleFreeFlyCamera(GameplayState& state, entt::registry& registry)
    {
        if (state.controlMode == ControlMode::FreeFlyCamera)
        {
            state.controlMode = ControlMode::Player;
            return;
        }

        // The free-fly camera jumps to the eyes of the player and looks the same way. Its previous transform is set too,
        // so the interpolation does not draw it flying from its old place in the first frame.
        const LookAngles& look = registry.get<LookAngles>(state.player);
        const Core::Transform eyes = CalculatePlayerEyeTransform(registry.get<Core::Transform>(state.player), look);
        registry.get<Core::Transform>(state.freeFlyCamera) = eyes;
        registry.get<Core::PreviousTransform>(state.freeFlyCamera).value = eyes;
        registry.get<LookAngles>(state.freeFlyCamera) = look;

        state.controlMode = ControlMode::FreeFlyCamera;
    }

    Core::Transform CalculateViewTransform(const GameplayState& state, const entt::registry& registry, float interpolationFactor)
    {
        const entt::entity viewEntity = GetViewEntity(state);
        const Core::Transform interpolated = Core::InterpolateTransform(
            registry.get<Core::PreviousTransform>(viewEntity).value, registry.get<Core::Transform>(viewEntity),
            interpolationFactor);

        // The body of the player does not turn with the view: the eyes take the look angles, which follow the mouse every
        // frame and so are not interpolated.
        if (state.controlMode == ControlMode::Player)
        {
            // The step smoothing moves in ticks too, so it is drawn between its last two values like the transform.
            const auto& smoothing = registry.get<PlayerStepSmoothing>(state.player);
            const float stepOffset = glm::mix(smoothing.previousOffset, smoothing.offset, interpolationFactor);
            return CalculatePlayerEyeTransform(interpolated, registry.get<LookAngles>(state.player), stepOffset);
        }

        return interpolated;
    }

    const Renderer::CameraLens& GetViewLens(const GameplayState& state, const entt::registry& registry)
    {
        return registry.get<Renderer::CameraLens>(GetViewEntity(state));
    }
}
