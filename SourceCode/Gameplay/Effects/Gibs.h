#pragma once

#include "Audio/AudioEngine.h"
#include "Audio/SoundEvent.h"
#include "Core/Math/Random.h"
#include "Renderer/Assets/ModelStore.h"
#include "Renderer/Assets/ShaderStore.h"
#include "World/CollisionBrush.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace Abomination::Renderer
{
    struct RenderAssets;
}

// The gibs: a body torn apart by shots bursts into chunks of meat that fly out with blood, bounce off the level, lie
// on the floor for a while and sink into it, like in Quake. Only for the eyes and the ears: nothing in the game depends
// on them, so they move every frame, like the shells.
namespace Abomination::Gameplay
{
    struct GameplayState;

    // Values a designer tunes (in the Enemies window). Speeds in meters per second, times in seconds.
    struct GibSettings
    {
        // A body bursts once the damage it took beyond death reaches this (see Health::overkill): 40, like the -40
        // health of Quake. A close shot at a wounded dog does it, or a shot at its body.
        float burstDamage = 40.0f;

        // How many chunks a body bursts into (the three models in turn), how fast they fly out (in all directions,
        // more upwards, and along the shot that tore the body), and how fast they tumble.
        int count = 7;
        float speed = 3.5f;
        float upSpeed = 3.0f;
        float shotSpeed = 2.0f;
        float spinSpeed = 12.0f;

        // A bounce: the part of the speed into the surface a chunk keeps (meat hardly bounces), and of the speed along
        // the surface (rubbing). Slower than restSpeed on a floor, it lies down.
        float bounce = 0.2f;
        float slide = 0.45f;
        float restSpeed = 0.6f;

        // How many bursts (groups of chunks) lie in the level at most: a new one beyond this makes the whole oldest group
        // sink into the floor, like the bodies (see Corpse). There is no time limit: the chunks lie until then.
        int maximumGroups = 8;
    };

    // One chunk: an entity drawn as one of the gib models, and how it moves.
    struct Gib
    {
        entt::entity entity = entt::null;
        glm::vec3 velocity{0.0f};
        glm::vec3 spinAxis{1.0f, 0.0f, 0.0f};
        float spinSpeed = 0.0f;

        // The burst it came from (the smaller, the older); it lies still; it sinks into the floor, and how deep it is.
        std::uint64_t group = 0;
        bool isResting = false;
        bool isSinking = false;
        float sunkDepth = 0.0f;
    };

    // Everything the gibs need between frames.
    struct Gibs
    {
        GibSettings settings;

        // The chunks in the level, and the number of the next burst.
        std::vector<Gib> gibs;
        std::uint64_t nextGroup = 0;

        std::array<Renderer::ModelHandle, 3> models;
        Renderer::ShaderHandle shaderProgram;
        Audio::SoundEventHandle burstSound;
        Core::Random random;
    };

    // Loads the gib models and the sound of a burst, for the whole game.
    [[nodiscard]] Gibs LoadGibs(Renderer::RenderAssets& renderAssets, Audio::AudioEngine& audio);

    // A body at center, halfExtents in size, bursts: the sound, blood all around, and the chunks flying out, pushed
    // along shotDirection (the way the last shot went; zero for none).
    void BurstIntoGibs(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, const glm::vec3& center,
                       const glm::vec3& halfExtents, const glm::vec3& shotDirection);

    // Once per frame (deltaTime, seconds): the chunks fall, tumble, bounce off the level and lie down; the chunks of old
    // groups sink into the floor and are gone.
    void UpdateGibs(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                    float deltaTime);

    // Removes every chunk (when the level is replaced).
    void ClearGibs(Gibs& gibs, entt::registry& registry);
}
