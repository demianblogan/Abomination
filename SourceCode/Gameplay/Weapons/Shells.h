#pragma once

#include "Audio/AudioEngine.h"
#include "Audio/SoundEvent.h"
#include "Core/Math/Random.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/Effects/Tumbling.h"
#include "Renderer/Assets/MeshStore.h"
#include "Renderer/Assets/ShaderStore.h"
#include "Renderer/Material.h"
#include "World/CollisionBrush.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace Abomination::Renderer
{
    struct RenderAssets;
}

// The spent shells the shotgun throws out of its window when the pump goes back: they fly, spin, bounce off the level
// and stay lying on the floor until newer shells take their place. Only for the eyes and the ears: nothing in the game
// depends on them, so they move every frame, like the particles.
namespace Abomination::Gameplay
{
    struct GameplayState;

    // Values a designer tunes (in the Shells tab of the Weapon window). Speeds in meters per second, times in seconds.
    struct ShellSettings
    {
        // How the shell is thrown out, in the directions of the weapon (it turns with the weapon, so a weapon tilted to the
        // chest throws it up): to the right side of the weapon, up and back towards the stock. Each throw changes the
        // speed by up to speedVariation (0.2: by up to 20%) and the direction a little.
        float sideSpeed = 2.0f;
        float upSpeed = 0.85f;
        float backSpeed = 0.8f;
        float speedVariation = 0.2f;

        // How fast it spins end over end while it flies (radians per second).
        float spinSpeed = 18.0f;

        // Where it comes out, relative to the place of the window found on the model (WeaponViewModel::shellWindow), in
        // meters along the weapon's own directions: right, up, back. Tuned by eye to where the window is seen.
        glm::vec3 windowOffset{-0.025f, 0.0f, 0.1f};

        // A bounce: the part of the speed into the surface it keeps (0.35: it jumps back with 35% of it), and the part of
        // the speed along the surface it keeps (rubbing).
        float bounce = 0.35f;
        float slide = 0.6f;

        // Slower than this (meters per second) on a floor, it stops and lies down.
        float restSpeed = 0.5f;

        // How many shells lie in the level at most: a new one beyond this takes the place of the oldest.
        int maximumCount = 20;

        // The smoke out of the window when the shell is thrown: a few puffs that rise slowly and fade.
        int windowSmokeCount = 3;
        float windowSmokeLifetime = 0.6f;
        float windowSmokeHalfSize = 0.05f;

        // The thin smoke the shell trails while it flies, one puff every trailInterval for trailDuration after the throw.
        float trailInterval = 0.025f;
        float trailDuration = 0.5f;
        float trailLifetime = 0.4f;
        float trailHalfSize = 0.018f;

        // The sound of a bounce is heard from this speed into the surface (meters per second); at fullVolumeSpeed and
        // faster at full volume.
        float soundSpeed = 0.4f;
        float fullVolumeSpeed = 3.0f;

        // How long the ringing of the last bounce fades out once the shell lies still.
        float soundFadeOut = 0.15f;
    };

    // One shell: an entity drawn as the shell mesh (see Shells), and how it moves.
    struct Shell
    {
        entt::entity entity = entt::null;

        // How it flies and spins (see Tumbling.h).
        Tumbler motion;

        // Seconds since it was thrown, and when it trails the next puff of smoke.
        float age = 0.0f;
        float nextTrailTime = 0.0f;

        // It lies on the floor and no longer moves.
        bool isResting = false;

        // The sound of its last bounce. A recording of a fall rings on after the hit; once the shell lies still, the
        // ringing is faded out (it would sound like a shell still rolling).
        Audio::VoiceId voice;
    };

    // Everything the shells need between frames: the shells in the level and what they are drawn with.
    struct Shells
    {
        ShellSettings settings;

        // The shells, as a ring: once it holds maximumCount of them, a new one replaces the one at nextReplaced (the
        // oldest), and nextReplaced goes on to the next. No entity is made or destroyed in a fight once the ring is full.
        std::vector<Shell> shells;
        std::size_t nextReplaced = 0;

        Renderer::MeshHandle mesh;
        Renderer::Material material;
        Renderer::ShaderHandle shaderProgram;
        Audio::SoundEventHandle dropSound;

        Core::Random random;
    };

    // The size of a 12-gauge shell (meters): about 2 cm across and 7 cm long.
    inline constexpr float ShellRadius = 0.0105f;
    inline constexpr float ShellLength = 0.07f;

    // Makes the shell mesh, loads its material and the sound of its fall, for the whole game.
    [[nodiscard]] Shells LoadShells(Renderer::RenderAssets& renderAssets, Audio::AudioEngine& audio);

    // Throws a shell out of the window of the weapon in the hands. eyes: where the player's eyes are and how they are
    // turned (the weapon is placed relative to them); playerVelocity: how fast the player moves (the shell keeps it).
    void EjectShell(GameplayState& state, entt::registry& registry, const Core::Transform& eyes,
                    const glm::vec3& playerVelocity, std::span<const World::CollisionBrush> brushes);

    // Once per frame (deltaTime, seconds): throws a shell if the pump has just reached the back (see PumpAction), moves
    // the flying shells through the level, bounces them, lays them down, trails their smoke and plays their sounds.
    void UpdateShells(GameplayState& state, entt::registry& registry, const Core::Transform& eyes,
                      std::span<const World::CollisionBrush> brushes, Audio::AudioEngine& audio, float deltaTime);

    // Removes every shell (when the level is replaced, or from the debug overlay).
    void ClearShells(Shells& shells, entt::registry& registry);
}
