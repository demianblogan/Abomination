#pragma once

#include "Core/Math/Random.h"
#include "Core/Math/Units.h"
#include "Gameplay/Enemies/DogMind.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <span>
#include <vector>

namespace Abomination::Audio
{
    class AudioEngine;
}

namespace Abomination::Renderer
{
    class DebugLines;
    struct RenderAssets;
}

namespace Abomination::World
{
    struct CollisionBrush;
    struct MonsterStart;
}

// The monsters of the level: created from the monster_* entities of the map (see World::MonsterStart), moved through the
// level by the same code as the player (Physics::UpdateCharacter), hurt and killed by the weapons. The first one is the
// dog (monster_dog).
namespace Abomination::Gameplay
{
    struct GameplayState;

    // Half the size of the box of the dog: 34 x 24 x 34 units, 1.06 m across and 0.75 m tall. A box does not turn, so it
    // is as wide as the dog is long (1.1 m) whichever way it faces: its muzzle and tail stay out of the walls. The same box
    // is set for TrenchBroom (Abomination.fgd).
    inline constexpr glm::dvec3 DogHalfExtents{Core::MapUnitsToMeters(17.0), Core::MapUnitsToMeters(12.0),
                                               Core::MapUnitsToMeters(17.0)};

    // Component of a dog: its mind (see DogMind.h) and what it remembers between ticks to notice changes.
    struct Dog
    {
        float maximumHealth = 60.0f;
        DogMind mind;

        // Its health and the count of the player's shots in the last tick: less health means it was hurt, more shots
        // that a shot was fired. -1 until its first tick: a dog spawned after shots (a level reload) must not take the
        // shots fired before it lived for a new one.
        float lastHealth = 60.0f;
        int lastShotCount = -1;

        // Where it appeared (the center of its body then): it patrols around it. How long it has wanted to move but
        // hardly moved; long enough, and it is blocked (see DogPerception::isBlocked).
        glm::vec3 home{0.0f};
        float blockedTime = 0.0f;
        bool isBlocked = false;

        // Its own random numbers, so dogs placed together do not all wander the same way.
        Core::Random random;
    };

    // Creates a monster at every start the game knows (monster_dog); other class names are logged and skipped.
    [[nodiscard]] std::vector<entt::entity> SpawnMonsters(entt::registry& registry, Renderer::RenderAssets& assets,
                                                          std::span<const World::MonsterStart> starts);

    // Destroys the entities of the monsters (before the level is replaced).
    void DestroyMonsters(entt::registry& registry, std::span<const entt::entity> monsters);

    // A monster was killed. Until the deaths of feat/death it simply disappears.
    void KillMonster(entt::registry& registry, entt::entity monster);

    // Once per tick: every dog perceives the player (sight, the sound of shots, being hurt), decides what to do (see
    // UpdateDogMind) and does it: moves through the level like a character (sliding along walls, up steps, falling),
    // turns, plays the clip of its state and bites. state.dogSettings tunes all dogs.
    void UpdateMonsters(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                        Audio::AudioEngine& audio, float tickDuration);

    // Debug, with state.areDogSensesVisible: the senses of every dog as lines on the floor around it: its field of view
    // and the range of its sight (yellow), the radius it smells the player in (orange), the range it hears shots in
    // (blue) and the area it patrols around where it appeared (green). interpolationFactor places it as it is drawn.
    void AddMonsterDebugLines(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                              Renderer::DebugLines& lines);
}
