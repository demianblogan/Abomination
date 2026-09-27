#include "Gameplay/Player.h"

#include "Core/Name.h"
#include "Core/TransformInterpolation.h"
#include "Gameplay/MouseLook.h"
#include "Physics/CharacterBody.h"
#include "Renderer/CameraLens.h"
#include "World/CollisionDebug.h"

#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
    entt::entity SpawnPlayer(entt::registry& registry, const World::PlayerStart& playerStart)
    {
        const entt::entity player = registry.create();
        registry.emplace<Core::Name>(player, "Player");

        // The map gives the eyes; the box is centered PlayerEyeHeight below them. For a player start placed on the floor
        // in TrenchBroom (its box of 32 x 56 x 32 units standing on it) the bottom of the box is exactly on the floor.
        const glm::vec3 center = playerStart.eyePosition - glm::vec3(0.0f, PlayerEyeHeight, 0.0f);
        registry.emplace<Core::Transform>(player, Core::Transform{.position = center});
        registry.emplace<Physics::CharacterBody>(player, Physics::CharacterBody{.halfExtents = World::PlayerHalfExtents});
        registry.emplace<PlayerLook>(player, PlayerLook{.yaw = playerStart.yaw});
        registry.emplace<Renderer::CameraLens>(player);
        Core::EnableInterpolation(registry, player);

        return player;
    }

    Core::Transform CalculatePlayerEyeTransform(const Core::Transform& body, const PlayerLook& look)
    {
        return Core::Transform{
            .position = body.position + glm::vec3(0.0f, PlayerEyeHeight, 0.0f),
            .rotation = CalculateCameraRotation(look.yaw, look.pitch),
        };
    }
}
