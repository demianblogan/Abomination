#pragma once

#include "Core/Math/Random.h"
#include "Core/Math/Units.h"
#include "Gameplay/Enemies/DogMind.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <span>
#include <vector>

namespace Abomination::Audio
{
    class AudioEngine;
}

namespace Abomination::Navigation
{
    class NavMesh;
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

        // The time left until its next bark in a chase.
        float barkTimer = 0.0f;

        // The way it runs (chase) or walks (patrol) around walls: the corners of its path on the navmesh, the first one
        // where it was when the path was found; empty while the way is straight. The time until the path is found
        // again (the player moves).
        std::vector<glm::vec3> path;
        float repathTimer = 0.0f;

        // Its own random numbers, so dogs placed together do not all wander the same way.
        Core::Random random;
    };

    // Component of a dead monster: its body lies where it died, in the last pose of its death clip. Characters walk
    // through it, shots still hit it. Only the newest bodies are kept (GameplayState::maximumCorpses): an older one
    // sinks into the floor and is gone.
    struct Corpse
    {
        // The order of the deaths: the smallest number is the oldest body.
        std::uint64_t order = 0;

        // How far it has sunk into the floor (meters), once it sinks.
        bool isSinking = false;
        float sunkDepth = 0.0f;

        // How far its box was lowered when it lay down (see LowerCorpse): its model is drawn that much higher.
        float raise = 0.0f;
    };

    // How fast an old body sinks into the floor: through its whole height in about a second.
    inline constexpr float CorpseSinkSpeed = 0.8f;

    // Creates a monster at every start the game knows (monster_dog); other class names are logged and skipped.
    [[nodiscard]] std::vector<entt::entity> SpawnMonsters(entt::registry& registry, Renderer::RenderAssets& assets,
                                                          std::span<const World::MonsterStart> starts);

    // Destroys the entities of the monsters (before the level is replaced).
    void DestroyMonsters(entt::registry& registry, std::span<const entt::entity> monsters);

    // Once per tick: first the dogs with no health left (killed by a shot) die: they yelp, play their death clip and
    // become bodies (see Corpse); the bodies fall, slide when shot, and the oldest sink when there are too many. Then
    // every dog perceives the player (sight, the sound of shots, being hurt), decides what to do (see UpdateDogMind) and
    // does it: moves through the level like a character (sliding along walls, up steps, falling), turns, plays the clip
    // of its state and bites; it barks, bites, lands and yelps with the sounds of state.dogSounds. state.dogSettings
    // tunes all dogs.
    // brushes are what the dogs walk on and against (clip included), sightBrushes what hides the player (no clip),
    // navMesh where they find their way around walls (none: straight at the player).
    void UpdateMonsters(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                        std::span<const World::CollisionBrush> sightBrushes, const Navigation::NavMesh* navMesh,
                        Audio::AudioEngine& audio, float tickDuration);

    // Debug, with state.areDogSensesVisible: the senses of every dog as lines on the floor around it: its field of view
    // and the range of its sight (yellow), the radius it smells the player in (orange), the range it hears shots in
    // (blue) and the area it patrols around where it appeared (green). interpolationFactor places it as it is drawn.
    void AddMonsterDebugLines(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                              Renderer::DebugLines& lines);

    // Debug, with state.isNavMeshVisible: the polygons of the navmesh as lines a little above the floor (none if the level
    // has no navmesh).
    void AddNavMeshDebugLines(const GameplayState& state, const Navigation::NavMesh* navMesh, Renderer::DebugLines& lines);
}
