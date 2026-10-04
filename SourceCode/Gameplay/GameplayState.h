#pragma once

#include "Audio/SoundEvent.h"
#include "Gameplay/Camera/FreeFlyCameraController.h"
#include "Gameplay/Characters/DamageKind.h"
#include "Gameplay/Effects/Effects.h"
#include "Gameplay/Effects/Gibs.h"
#include "Gameplay/Enemies/DogMind.h"
#include "Gameplay/Player/PlayerController.h"
#include "Gameplay/Player/PlayerDeath.h"
#include "Gameplay/Weapons/Shells.h"
#include "Physics/CharacterMovement.h"

#include <entt/entt.hpp>

#include <array>
#include <cstdint>
#include <vector>

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
        Audio::SoundEventHandle jump;
        Audio::SoundEventHandle land;

        // The voice of the player when hurt, when killed and when healed (a sigh of relief), the sound of a blow by its
        // kind, and the beat of the heart at low health (see DamageReaction).
        Audio::SoundEventHandle hurt;
        Audio::SoundEventHandle death;
        Audio::SoundEventHandle relief;
        std::array<Audio::SoundEventHandle, DamageKindCount> hits;
        Audio::SoundEventHandle heartbeat;

        // The chord of GAME OVER after the death (see PlayerDeath).
        Audio::SoundEventHandle gameOver;

        // The player stood on the ground in the last tick, and their vertical speed then (meters per second, negative
        // while falling). A landing is noticed one tick later, when the physics has already stopped the fall, so the
        // speed of the fall is taken from here.
        bool wasOnGround = true;
        float previousVerticalSpeed = 0.0f;
    };

    // The sounds of the dogs, played where the dog is (see UpdateMonsters): a bark when it notices the player, the bite
    // that hurts, the landing of a leap, a yelp when hit, and its death.
    struct DogSounds
    {
        Audio::SoundEventHandle bark;
        Audio::SoundEventHandle bite;
        Audio::SoundEventHandle land;
        Audio::SoundEventHandle hurt;
        Audio::SoundEventHandle death;
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

        // The death of the player: the fall of the view, the fade to black, GAME OVER (see PlayerDeath.h).
        PlayerDeath playerDeath;

        PlayerSounds playerSounds;
        DogSounds dogSounds;

        // The monsters of the level (see Monsters.h); replaced when the level is.
        std::vector<entt::entity> monsters;

        // How every dog sees, hears, moves and bites (tuned in the Enemy window, see DogMind.h).
        DogSettings dogSettings;

        // At most this many bodies of dead monsters stay in the level (tuned in the Enemies window); the next death to
        // number a body by (see Corpse::order).
        int maximumCorpses = 16;
        std::uint64_t nextCorpseOrder = 0;

        // Debug (the Enemies window): the monsters stand and decide nothing; the senses of the dogs are drawn; the navmesh
        // is drawn.
        bool areMonstersFrozen = false;
        bool areDogSensesVisible = false;
        bool isNavMeshVisible = false;

        // The visual effects of shots (see Effects.h).
        Effects effects;

        // The spent shells thrown out by the pump (see Shells.h).
        Shells shells;

        // The chunks of bodies torn apart by shots (see Gibs.h).
        Gibs gibs;
    };

    // Tells the model store how the gameplay needs its models split (the bolt of the shotgun is taken out of its body, see
    // ModelPartSplit). Called before anything loads them: the level may show the shotgun before the player gets it.
    void PrepareGameplayModels(Renderer::RenderAssets& renderAssets);

    // Creates the player at the player start of the level and the free-fly camera waiting at their eyes, looking the
    // same way, loads the sounds of the player and gives them the shotgun in their hands (see WeaponViewModel). The player is
    // controlled first.
    [[nodiscard]] GameplayState CreateGameplayState(entt::registry& registry, const World::PlayerStart& playerStart,
                                                    Audio::AudioEngine& audio, Renderer::RenderAssets& renderAssets);
}
