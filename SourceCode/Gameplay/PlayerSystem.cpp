#include "Gameplay/PlayerSystem.h"

#include "Audio/AudioEngine.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Player.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/Debug/DebugLines.h"

#include <glm/common.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace Abomination::Gameplay
{
    namespace
    {
        // The box of the player, drawn while the free-fly camera is used.
        constexpr glm::vec3 PlayerBoxColor{0.3f, 1.0f, 0.5f};
    }

    void UpdatePlayerLook(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                          glm::vec2 mouseMovement)
    {
        if (state.controlMode != ControlMode::Player)
            return;

        state.playerController.CollectFrameInput(actions);
        state.playerController.UpdateRotation(registry.get<LookAngles>(state.player), mouseMovement);
    }

    void UpdatePlayer(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                      std::span<const World::CollisionBrush> brushes, float tickDuration)
    {
        const Physics::MoveCommand command =
            state.controlMode == ControlMode::Player
                ? state.playerController.CreateMoveCommand(registry.get<LookAngles>(state.player), actions)
                : Physics::MoveCommand{};

        Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(state.player);
        Physics::UpdateCharacter(body, registry.get<Core::Transform>(state.player), brushes, state.physicsSettings,
                                 state.movementSettings, command, tickDuration);
        UpdateStepSmoothing(registry.get<PlayerStepSmoothing>(state.player), body.steppedUpHeight, tickDuration);
    }

    float CalculateLandingVolume(float fallSpeed)
    {
        if (fallSpeed < MinimumLandingSoundSpeed)
            return 0.0f;

        // 0 at the quietest audible landing, 1 at FullLandingSoundSpeed and faster.
        const float loudness = glm::clamp((fallSpeed - MinimumLandingSoundSpeed) /
                                          (FullLandingSoundSpeed - MinimumLandingSoundSpeed), 0.0f, 1.0f);
        constexpr float QuietestVolume = 1.0f / 3.0f;

        return glm::mix(QuietestVolume, 1.0f, loudness);
    }

    void UpdatePlayerSounds(GameplayState& state, const entt::registry& registry, Audio::AudioEngine& audio)
    {
        PlayerSounds& sounds = state.playerSounds;
        const Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(state.player);

        // "In the head" while the player is controlled, at the player otherwise.
        const std::optional<glm::vec3> position =
            state.controlMode == ControlMode::Player
                ? std::nullopt
                : std::optional<glm::vec3>(registry.get<Core::Transform>(state.player).position);

        // A jump leaves the ground upwards; walking off an edge leaves it with no upward speed.
        if (sounds.wasOnGround && !body.isOnGround && body.velocity.y > 0.0f)
            audio.Play(sounds.jump, position);

        if (!sounds.wasOnGround && body.isOnGround)
        {
            const float volume = CalculateLandingVolume(-sounds.previousVerticalSpeed);
            if (volume > 0.0f)
            {
                Audio::SoundEvent landing = sounds.land;
                landing.volume *= volume;
                audio.Play(landing, position);
            }
        }

        sounds.wasOnGround = body.isOnGround;
        sounds.previousVerticalSpeed = body.velocity.y;
    }

    void AddPlayerDebugBox(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                           Renderer::DebugLines& debugLines)
    {
        if (state.controlMode != ControlMode::FreeFlyCamera)
            return;

        const glm::vec3 center = Core::InterpolateTransform(registry.get<Core::PreviousTransform>(state.player).value,
                                                            registry.get<Core::Transform>(state.player),
                                                            interpolationFactor).position;
        const glm::vec3 halfExtents(registry.get<Physics::CharacterBody>(state.player).halfExtents);
        debugLines.AddBox(center - halfExtents, center + halfExtents, PlayerBoxColor);
    }
}
