#pragma once

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <span>

namespace Abomination::Audio
{
    class AudioEngine;
}

namespace Abomination::World
{
    struct CollisionBrush;
}

// The bodies of dead monsters, whatever kind they were: a body lies where the monster died, characters walk through it,
// shots still hit it and push it; enough damage beyond death bursts it into gibs; only the newest bodies are kept.
namespace Abomination::Gameplay
{
    struct GameplayState;

    // Component of a dead monster: its body lies where it died, in the last pose of its death clip. Only the newest
    // bodies are kept (GameplayState::maximumCorpses): an older one sinks into the floor and is gone.
    struct Corpse
    {
        // The order of the deaths: the smallest number is the oldest body.
        std::uint64_t order = 0;

        // It sinks into the floor, and how far it has (meters).
        bool isSinking = false;
        float sunkDepth = 0.0f;

        // How far its box was lowered when it lay down (see LayDownCorpse): its model is drawn that much higher.
        float raise = 0.0f;
    };

    // How fast an old body sinks into the floor: through its whole height in about a second.
    inline constexpr float CorpseSinkSpeed = 0.8f;

    // The monster entity is dead and becomes a body: a Corpse, numbered after the bodies before it, and its box lowered
    // to half its height (the bottom where it was), so shots above the body pass over it. Its own component (Dog, ...)
    // is the caller's to remove, with its death sound and clip.
    void LayDownCorpse(GameplayState& state, entt::registry& registry, entt::entity entity);

    // A body (alive a moment ago, or already lying) that took enough damage beyond death (see Health::overkill and
    // GibSettings::burstDamage) bursts into gibs, flying away from shotFrom (the eyes of the player, who shot it), and is
    // gone. Tells whether it burst.
    bool BurstIfTornApart(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, entt::entity entity,
                          const glm::vec3& shotFrom);

    // Once per tick: bodies shot to pieces burst; the bodies fall and slide through the level (not against characters);
    // when there are more than state.maximumCorpses, the oldest sink into the floor and are gone.
    void UpdateCorpses(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                       Audio::AudioEngine& audio, const glm::vec3& shotFrom, float tickDuration);
}
