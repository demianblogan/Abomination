#pragma once

#include "Audio/SoundEvent.h"
#include "Gameplay/FreeFlyCameraController.h"
#include "Gameplay/PlayerController.h"
#include "Physics/CharacterMovement.h"

#include <entt/entt.hpp>

namespace Abomination::Audio
{
    class AudioEngine;
}

namespace Abomination::Renderer
{
    struct RenderAssets;
}

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

    // The sounds of the player and what is needed to notice when to play them (see UpdatePlayerSounds).
    struct PlayerSounds
    {
        // The voice of the player jumping (an effort sound) and the feet landing on the ground.
        Audio::SoundEvent jump;
        Audio::SoundEvent land;

        // The player stood on the ground in the last tick, and their vertical speed then (meters per second, negative
        // while falling). A landing is noticed one tick later, when the physics has already stopped the fall, so the
        // speed of the fall is taken from here.
        bool wasOnGround = true;
        float previousVerticalSpeed = 0.0f;
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

        PlayerSounds playerSounds;
    };

    // Creates the player at the player start of the level and the free-fly camera waiting at their eyes, looking the
    // same way, loads the sounds of the player and gives them the shotgun in their hands (see ViewModel). The player is
    // controlled first.
    [[nodiscard]] GameplayState CreateGameplayState(entt::registry& registry, const World::PlayerStart& playerStart,
                                                    Audio::AudioEngine& audio, Renderer::RenderAssets& renderAssets);
}
