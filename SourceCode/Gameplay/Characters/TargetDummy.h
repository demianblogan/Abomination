#pragma once

#include "Renderer/Assets/ModelStore.h"
#include "Renderer/Assets/ShaderStore.h"

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
    struct PlayerStart;
}

// A temporary target to shoot at until the first enemy (0.4), then removed with these files: TargetDummy.h/.cpp, its
// use in WeaponSystem.cpp and GameplayState, target_dummy in Level and the FGD, the Dummy model.
namespace Abomination::Gameplay
{
    // Component of a target dummy: a character with the box of the player that stands, takes damage (it has a Health),
    // is pushed by shots and slides along the floor like the player. When destroyed it disappears and comes back to where
    // it started after respawnDelay, as soon as nobody stands there.
    struct TargetDummy
    {
        // Where it appears (the center of its box) and which way it faces (yaw, radians).
        glm::vec3 spawnCenter{0.0f};
        float spawnYaw = 0.0f;

        float maximumHealth = 100.0f;
        float respawnDelay = 3.0f;

        // Destroyed and waiting to come back, and for how many more seconds.
        bool isDestroyed = false;
        float respawnTimer = 0.0f;

        // What draws it, kept while it is destroyed (its ModelRenderer is removed then) to put it back.
        Renderer::ModelHandle model;
        Renderer::ShaderHandle shaderProgram;
    };

    // Creates a target dummy at every start (see World::Level::GetTargetDummyStarts).
    [[nodiscard]] std::vector<entt::entity> SpawnTargetDummies(entt::registry& registry, Renderer::RenderAssets& assets,
                                                               std::span<const World::PlayerStart> starts);

    // Destroys the entities of the dummies (before the level is replaced).
    void DestroyTargetDummies(entt::registry& registry, std::span<const entt::entity> dummies);

    // A dummy was killed: it disappears (no model, no body) and starts waiting to come back.
    void DestroyTargetDummy(entt::registry& registry, entt::entity dummy);

    // Once per tick: dummies that stand move through the level like characters with no command (they slide after a
    // shot and stop by friction, fall when pushed off a ledge); destroyed ones count down and come back.
    void UpdateTargetDummies(entt::registry& registry, const Renderer::RenderAssets& assets,
                             std::span<const World::CollisionBrush> brushes, const Physics::PhysicsSettings& physicsSettings,
                             const Physics::MovementSettings& movementSettings, float tickDuration);
}
