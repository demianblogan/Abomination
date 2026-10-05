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

namespace Abomination::Navigation
{
    class NavMesh;
}

namespace Abomination::Renderer
{
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

    // How much bigger the dog is drawn and collides than its model was made (Tools/Blender/RigDog.py, 1.1 m long).
    inline constexpr float DogScale = 1.2f;

    // Half the size of the box of the dog: 34 x 24 x 34 units at the size of its model, times DogScale: 41 x 29 x 41
    // units, 1.28 m across and 0.9 m tall. A box does not turn, so it is as wide as the dog is long whichever way it
    // faces: its muzzle and tail stay out of the walls. The same box is set for TrenchBroom (Abomination.fgd), and the
    // navmesh is built for it (Navigation::NavMeshSettings).
    inline constexpr glm::dvec3 DogHalfExtents{Core::MapUnitsToMeters(17.0 * DogScale),
                                               Core::MapUnitsToMeters(12.0 * DogScale),
                                               Core::MapUnitsToMeters(17.0 * DogScale)};

    // Component of a dog: its mind (see DogMind.h) and what it remembers between ticks to notice changes.
    struct Dog
    {
        float maximumHealth = 90.0f;
        DogMind mind;

        // Its health and the count of the player's shots in the last tick: less health means it was hurt, more shots
        // that a shot was fired. -1 until its first tick: a dog spawned after shots (a level reload) must not take the
        // shots fired before it lived for a new one.
        float lastHealth = 90.0f;
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

        // Its path ends short of the goal, which it cannot reach (the player on the altar, above it).
        bool isPathShort = false;

        // Its own random numbers, so dogs placed together do not all wander the same way.
        Core::Random random;
    };

    // Creates a monster at every start the game knows (monster_dog); other class names are logged and skipped.
    [[nodiscard]] std::vector<entt::entity> SpawnMonsters(entt::registry& registry, Renderer::RenderAssets& assets,
                                                          std::span<const World::MonsterStart> starts);

    // Destroys the entities of the monsters (before the level is replaced).
    void DestroyMonsters(entt::registry& registry, std::span<const entt::entity> monsters);

    // Once per tick: first the dogs with no health left (killed by a shot) die: they yelp, play their death clip and
    // become bodies, or burst into gibs (see Corpses.h); then the bodies are updated. Then every dog perceives the
    // player (sight, the sound of shots, being hurt), finds its way, decides what to do (see UpdateDogMind) and does it:
    // moves through the level like a character (sliding along walls, up steps, falling), turns, plays the clip of its
    // state and bites; it barks, bites, lands and yelps with the sounds of state.dogSounds. state.dogSettings tunes all
    // dogs. brushes are what the dogs walk on and against (clip included), sightBrushes what hides the player (no clip),
    // navMesh where they find their way around walls (none: straight at the player).
    void UpdateMonsters(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                        std::span<const World::CollisionBrush> sightBrushes, const Navigation::NavMesh* navMesh,
                        Audio::AudioEngine& audio, float tickDuration);
}
