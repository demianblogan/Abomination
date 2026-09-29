#include "Gameplay/PlayerSystem.h"

#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Player.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/Debug/DebugLines.h"

#include <glm/vec3.hpp>

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
