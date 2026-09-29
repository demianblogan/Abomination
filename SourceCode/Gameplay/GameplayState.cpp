#include "Gameplay/GameplayState.h"

#include "Gameplay/Player.h"
#include "World/PlayerStart.h"

#include <glm/vec3.hpp>

namespace Abomination::Gameplay
{
    GameplayState CreateGameplayState(entt::registry& registry, const World::PlayerStart& playerStart)
    {
        GameplayState state;
        state.player = SpawnPlayer(registry, playerStart);

        const glm::vec3 eyePosition = playerStart.boxCenter + glm::vec3(0.0f, PlayerEyeHeight, 0.0f);
        state.freeFlyCamera = SpawnFreeFlyCamera(registry, eyePosition, playerStart.yaw);

        return state;
    }
}
