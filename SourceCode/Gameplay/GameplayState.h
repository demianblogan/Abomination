#pragma once

#include "Gameplay/FreeFlyCameraController.h"
#include "Gameplay/PlayerController.h"
#include "Physics/CharacterMovement.h"

#include <entt/entt.hpp>

namespace Abomination::World
{
    struct PlayerStart;
}

namespace Abomination::Gameplay
{
    // Who the input controls and whose eyes the scene is drawn through: the player, or the free-fly camera (F2).
    enum class ControlMode
    {
        Player,
        FreeFlyCamera,
    };

    // What the gameplay systems share between frames: the entities the input controls, the controllers that turn input
    // into what those entities do, and the settings of their movement. The components of the entities live in the
    // registry; this holds only the entity numbers. Application owns it and passes it to the systems (PlayerSystem.h,
    // FreeFlyCameraSystem.h, ViewSystem.h), which are free functions like every other system.
    struct GameplayState
    {
        ControlMode controlMode = ControlMode::Player;

        // The player, and the free-fly camera: a debug "noclip" view that flies through walls.
        entt::entity player = entt::null;
        entt::entity freeFlyCamera = entt::null;

        PlayerController playerController;
        FreeFlyCameraController freeFlyCameraController;

        // Gravity and other settings of the physical world, and how the player walks (speed, acceleration, friction,
        // steps). Changed in the Movement window of the debug overlay.
        Physics::PhysicsSettings physicsSettings;
        Physics::MovementSettings movementSettings;
    };

    // Creates the player at the player start of the level and the free-fly camera waiting at their eyes, looking the
    // same way. The player is controlled first.
    [[nodiscard]] GameplayState CreateGameplayState(entt::registry& registry, const World::PlayerStart& playerStart);
}
