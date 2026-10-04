#pragma once

#include "Core/Math/Units.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <span>
#include <vector>

namespace Abomination::Physics
{
    struct MovementSettings;
    struct PhysicsSettings;
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
    // Half the size of the box of the dog: 24 x 24 x 24 units, 0.75 m on every side. A box does not turn, so it is a
    // cube around the dog however it faces: its body (1.1 m long) reaches out of it a little in front and behind, which
    // is how Quake does it too. The same box is set for TrenchBroom (Abomination.fgd).
    inline constexpr glm::dvec3 DogHalfExtents{Core::MapUnitsToMeters(12.0), Core::MapUnitsToMeters(12.0),
                                               Core::MapUnitsToMeters(12.0)};

    // Component of a dog. Its mind comes in the next commit; for now it only stands, plays its clips and can be shot.
    struct Dog
    {
        float maximumHealth = 60.0f;
    };

    // Creates a monster at every start the game knows (monster_dog); other class names are logged and skipped.
    [[nodiscard]] std::vector<entt::entity> SpawnMonsters(entt::registry& registry, Renderer::RenderAssets& assets,
                                                          std::span<const World::MonsterStart> starts);

    // Destroys the entities of the monsters (before the level is replaced).
    void DestroyMonsters(entt::registry& registry, std::span<const entt::entity> monsters);

    // A monster was killed. Until the deaths of feat/death it simply disappears.
    void KillMonster(entt::registry& registry, entt::entity monster);

    // Once per tick: every monster moves through the level like a character (it slides after a shot and stops by
    // friction, falls when there is no floor).
    void UpdateMonsters(entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                        const Physics::PhysicsSettings& physicsSettings, const Physics::MovementSettings& movementSettings,
                        float tickDuration);
}
