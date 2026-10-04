#include "Gameplay/Player/PlayerSystem.h"

#include "Audio/AudioEngine.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Characters/CharacterCollision.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Player/PlayerDeath.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/Debug/DebugLines.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <cmath>
#include <optional>
#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        // The box of the player, drawn while the free-fly camera is used.
        constexpr glm::vec3 PlayerBoxColor{0.3f, 1.0f, 0.5f};

        // The boxes of the other characters: cyan, apart from the orange of the brushes.
        constexpr glm::vec3 CharacterBoxColor{0.2f, 0.85f, 1.0f};

        // How fast the player slides off the head of a character (meters per second, along the floor).
        constexpr float SlideOffSpeed = 4.0f;

        // The player may not stand on another character: on the back of a dog the dog could not reach them, and ran on
        // the spot under them. Standing on something that is not the level, they are pushed off it, away from the middle
        // of the character below, and fall.
        void SlideOffCharacters(GameplayState& state, entt::registry& registry,
                                std::span<const World::CollisionBrush> brushes, Physics::CharacterBody& body,
                                const Core::Transform& transform)
        {
            if (!body.isOnGround || Physics::IsOnGround(brushes, glm::dvec3(transform.position), body.halfExtents))
                return;

            const auto characters = registry.view<const Physics::CharacterBody, const Core::Transform>();
            for (const auto [entity, other, otherTransform] : characters.each())
            {
                if (entity == state.player)
                    continue;

                // The character under the player: their boxes overlap seen from above.
                const glm::vec3 offset = transform.position - otherTransform.position;
                const bool isBelow = std::abs(offset.x) < static_cast<float>(body.halfExtents.x + other.halfExtents.x) &&
                                     std::abs(offset.z) < static_cast<float>(body.halfExtents.z + other.halfExtents.z) &&
                                     offset.y > 0.0f;
                if (!isBelow)
                    continue;

                glm::vec3 away(offset.x, 0.0f, offset.z);
                const float length = glm::length(away);
                away = length > 1e-3f ? away / length : glm::vec3(1.0f, 0.0f, 0.0f);
                body.velocity.x = away.x * SlideOffSpeed;
                body.velocity.z = away.z * SlideOffSpeed;
                body.isOnGround = false;
                return;
            }
        }
    }

    void UpdatePlayerLook(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                          glm::vec2 mouseMovement)
    {
        // The dead do not look around (see PlayerDeath).
        if (state.controlMode != ControlMode::Player || IsPlayerDead(state))
            return;

        state.playerController.CollectFrameInput(actions);
        state.playerController.UpdateRotation(registry.get<LookAngles>(state.player), mouseMovement);
    }

    void UpdatePlayer(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                      std::span<const World::CollisionBrush> brushes, float tickDuration)
    {
        const Physics::MoveCommand command =
            state.controlMode == ControlMode::Player && !IsPlayerDead(state)
                ? state.playerController.CreateMoveCommand(registry.get<LookAngles>(state.player), actions)
                : Physics::MoveCommand{};

        // The player stops at the walls and at the other characters (the target dummies, later enemies).
        Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(state.player);
        const std::vector<World::CollisionBrush> obstacles = GatherCollisionBrushes(registry, brushes, state.player);
        Core::Transform& transform = registry.get<Core::Transform>(state.player);
        Physics::UpdateCharacter(body, transform, obstacles, state.physicsSettings, state.movementSettings, command,
                                 tickDuration);
        SlideOffCharacters(state, registry, brushes, body, transform);
        UpdateStepSmoothing(registry.get<StepSmoothing>(state.player), body.steppedUpHeight, tickDuration);
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
                audio.Play(sounds.land, position, false, volume);
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

        const glm::vec3 center = Core::CalculateDrawnTransform(registry, state.player, interpolationFactor).position;
        const glm::vec3 halfExtents(registry.get<Physics::CharacterBody>(state.player).halfExtents);
        debugLines.AddBox(center - halfExtents, center + halfExtents, PlayerBoxColor);
    }

    void AddCharacterDebugBoxes(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                                Renderer::DebugLines& debugLines)
    {
        for (const auto [entity, body, transform] : registry.view<const Physics::CharacterBody, const Core::Transform>().each())
        {
            if (entity == state.player)
                continue;

            // Drawn where the character is drawn: between its last two ticks.
            const glm::vec3 center = Core::CalculateDrawnTransform(registry, entity, interpolationFactor).position;
            const glm::vec3 halfExtents(body.halfExtents);
            debugLines.AddBox(center - halfExtents, center + halfExtents, CharacterBoxColor);
        }
    }
}
