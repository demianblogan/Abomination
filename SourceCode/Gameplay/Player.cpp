#include "Gameplay/Player.h"

#include "Core/Scene/Name.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/MouseLook.h"
#include "Physics/CharacterBody.h"
#include "Renderer/Camera/CameraLens.h"

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
    entt::entity SpawnPlayer(entt::registry& registry, const World::PlayerStart& playerStart)
    {
        const entt::entity player = registry.create();
        registry.emplace<Core::Name>(player, "Player");

        registry.emplace<Core::Transform>(player, Core::Transform{.position = playerStart.boxCenter});
        registry.emplace<Physics::CharacterBody>(player, Physics::CharacterBody{.halfExtents = World::PlayerHalfExtents});
        registry.emplace<LookAngles>(player, LookAngles{.yaw = playerStart.yaw});
        registry.emplace<PlayerStepSmoothing>(player);
        registry.emplace<Renderer::CameraLens>(player);
        Core::EnableInterpolation(registry, player);

        return player;
    }

    void UpdateStepSmoothing(PlayerStepSmoothing& smoothing, float steppedUpHeight, float deltaTime)
    {
        smoothing.previousOffset = smoothing.offset;

        // The body went up: the eyes stay where they were, so they are that much lower relative to the body now.
        smoothing.offset = glm::max(smoothing.offset - steppedUpHeight, -MaximumStepLag);

        // ...and rise towards the body at a constant speed, without overshooting.
        smoothing.offset = glm::min(smoothing.offset + StepSmoothingSpeed * deltaTime, 0.0f);
    }

    Core::Transform CalculatePlayerEyeTransform(const Core::Transform& body, const LookAngles& look, float stepOffset)
    {
        return Core::Transform{
            .position = body.position + glm::vec3(0.0f, PlayerEyeHeight + stepOffset, 0.0f),
            .rotation = CalculateCameraRotation(look),
        };
    }
}
