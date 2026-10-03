#include "Gameplay/Camera/ViewSystem.h"

#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Camera/MouseLook.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Player/LandingDip.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Weapons/ViewRecoil.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "Renderer/Camera/CameraLens.h"

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

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

    Core::Transform CalculateViewTransform(const GameplayState& state, const entt::registry& registry,
                                           float interpolationFactor)
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
            const auto& smoothing = registry.get<StepSmoothing>(state.player);
            const float stepOffset = glm::mix(smoothing.previousOffset, smoothing.offset, interpolationFactor);

            // The landing dip moves every frame, not in ticks, so it is used as it is.
            const LandingDip* dip = registry.try_get<LandingDip>(state.player);
            const float dipOffset = dip != nullptr ? dip->offset : 0.0f;

            // The recoil of a shot turns only the picture up, on top of the look angles (see ViewRecoil).
            LookAngles look = registry.get<LookAngles>(state.player);
            if (const ViewRecoil* recoil = registry.try_get<ViewRecoil>(state.player); recoil != nullptr)
                look.pitch += recoil->pitch;

            // A blow jerks the picture too: turned, and tilted around the direction of the view (see DamageReaction).
            const DamageReaction* reaction = registry.try_get<DamageReaction>(state.player);
            if (reaction != nullptr)
            {
                look.pitch += reaction->punchPitch;
                look.yaw += reaction->punchYaw;
            }

            Core::Transform eyes = CalculatePlayerEyeTransform(interpolated, look, stepOffset + dipOffset);
            if (reaction != nullptr)
                eyes.rotation = eyes.rotation * glm::angleAxis(reaction->punchRoll, Core::LocalForward);
            return eyes;
        }

        return interpolated;
    }

    const Renderer::CameraLens& GetViewLens(const GameplayState& state, const entt::registry& registry)
    {
        return registry.get<Renderer::CameraLens>(GetViewEntity(state));
    }

    void UpdateViewEffects(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, float deltaTime)
    {
        // The weapon swings with what the ticks of this frame did to the player (a landing, the speed) and with the view
        // turned this frame.
        UpdateWeaponViewModel(state, registry, audio, deltaTime);
        UpdatePlayerLandingDip(state, registry, deltaTime);
        UpdatePlayerViewRecoil(state, registry, deltaTime);
        state.effects.particles.Update(deltaTime, state.physicsSettings.gravity);
    }
}
