#include "Gameplay/TargetDummy.h"

#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Health.h"
#include "Gameplay/Player.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/DrawOffset.h"
#include "Renderer/ModelRenderer.h"
#include "World/PlayerStart.h"

#include <glm/gtc/quaternion.hpp>

namespace Abomination::Gameplay
{
    namespace
    {
        const std::string DummyModelPath = "Models/Enemies/Dummy.glb";

        // Puts the dummy where it starts, whole: at its spawn point, turned by its yaw, with full health, a body that
        // moves and its model scaled to the height of its box.
        void ResetDummy(entt::registry& registry, entt::entity entity, const TargetDummy& dummy,
                        const Renderer::RenderAssets& assets)
        {
            // The model is scaled evenly so its height is the height of the box (the box of the player, about 1.75 m).
            const Renderer::Model& model = assets.models.Get(dummy.model);
            const float boxHeight = static_cast<float>(World::PlayerHalfExtents.y * 2.0);
            const float scale = model.size.y > 0.0f ? boxHeight / model.size.y : 1.0f;

            const Core::Transform transform{
                .position = dummy.spawnCenter,
                .rotation = glm::angleAxis(dummy.spawnYaw, Core::WorldUp),
                .scale = glm::vec3(scale),
            };
            registry.emplace_or_replace<Core::Transform>(entity, transform);
            registry.emplace_or_replace<Core::PreviousTransform>(entity, Core::PreviousTransform{.value = transform});
            registry.emplace_or_replace<Physics::CharacterBody>(
                entity, Physics::CharacterBody{.halfExtents = World::PlayerHalfExtents});
            registry.emplace_or_replace<Health>(entity, Health{.current = dummy.maximumHealth, .maximum = dummy.maximumHealth});
            registry.emplace_or_replace<Renderer::ModelRenderer>(
                entity, Renderer::ModelRenderer{.model = dummy.model, .shaderProgram = dummy.shaderProgram});
            registry.emplace_or_replace<StepSmoothing>(entity);
            registry.emplace_or_replace<Renderer::DrawOffset>(entity);
        }
    }

    std::vector<entt::entity> SpawnTargetDummies(entt::registry& registry, Renderer::RenderAssets& assets,
                                                 std::span<const World::PlayerStart> starts)
    {
        std::vector<entt::entity> dummies;
        for (const World::PlayerStart& start : starts)
        {
            const entt::entity entity = registry.create();
            registry.emplace<Core::Name>(entity, "Target dummy");

            const TargetDummy& dummy = registry.emplace<TargetDummy>(entity, TargetDummy{
                .spawnCenter = start.boxCenter,
                .spawnYaw = start.yaw,
                .model = assets.LoadModel(DummyModelPath, Core::AssetLifetime::Level),
                .shaderProgram = assets.shaders.Load("Shaders/TexturedShaded"),
            });
            ResetDummy(registry, entity, dummy, assets);
            dummies.push_back(entity);
        }

        return dummies;
    }

    void DestroyTargetDummies(entt::registry& registry, std::span<const entt::entity> dummies)
    {
        for (const entt::entity entity : dummies)
            if (registry.valid(entity))
                registry.destroy(entity);
    }

    void DestroyTargetDummy(entt::registry& registry, entt::entity dummy)
    {
        TargetDummy* targetDummy = registry.try_get<TargetDummy>(dummy);
        if (targetDummy == nullptr || targetDummy->isDestroyed)
            return;

        // Without a model it is not drawn, without a body and health it is neither moved nor hit.
        registry.remove<Renderer::ModelRenderer, Physics::CharacterBody, Health>(dummy);
        targetDummy->isDestroyed = true;
        targetDummy->respawnTimer = targetDummy->respawnDelay;
    }

    void UpdateTargetDummies(entt::registry& registry, const Renderer::RenderAssets& assets,
                             std::span<const World::CollisionBrush> brushes, const Physics::PhysicsSettings& physicsSettings,
                             const Physics::MovementSettings& movementSettings, float tickDuration)
    {
        for (const auto [entity, dummy, transform] : registry.view<TargetDummy, Core::Transform>().each())
        {
            if (dummy.isDestroyed)
            {
                dummy.respawnTimer -= tickDuration;
                if (dummy.respawnTimer <= 0.0f)
                {
                    dummy.isDestroyed = false;
                    ResetDummy(registry, entity, dummy, assets);
                }
                continue;
            }

            // No command: it only slides after a push, slowed by friction, and falls when there is no floor.
            Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(entity);
            Physics::UpdateCharacter(body, transform, brushes, physicsSettings, movementSettings, Physics::MoveCommand{},
                                     tickDuration);

            // Pushed up a stair, the body jumps up at once; the model glides after it, like the eyes of the player.
            StepSmoothing& smoothing = registry.get<StepSmoothing>(entity);
            UpdateStepSmoothing(smoothing, body.steppedUpHeight, tickDuration);
            registry.replace<Renderer::DrawOffset>(entity, Renderer::DrawOffset{
                .offset = glm::vec3(0.0f, smoothing.offset, 0.0f),
                .previousOffset = glm::vec3(0.0f, smoothing.previousOffset, 0.0f),
            });
        }
    }
}
