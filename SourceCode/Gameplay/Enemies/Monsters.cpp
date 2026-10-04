#include "Gameplay/Enemies/Monsters.h"

#include "Core/Logging/Log.h"
#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Animation/Animator.h"
#include "Gameplay/Characters/CharacterCollision.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/Player/Player.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/DrawOffset.h"
#include "Renderer/ModelRenderer.h"
#include "World/MonsterStart.h"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <string>

namespace Abomination::Gameplay
{
    namespace
    {
        using Core::LogCategory;
        using Core::LogLevel;

        const std::string DogModelPath = "Models/Enemies/Dog.glb";

        entt::entity SpawnDog(entt::registry& registry, Renderer::RenderAssets& assets, const World::MonsterStart& start)
        {
            const entt::entity entity = registry.create();
            registry.emplace<Core::Name>(entity, "Dog");

            // The body stands on the origin of the map entity: its center is half its height above. The model is made to
            // the size of the dog (Tools/Blender/RigDog.py) and drawn with its middle at the middle of the box.
            const glm::vec3 center = start.origin + glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
            const Core::Transform transform{.position = center, .rotation = glm::angleAxis(start.yaw, Core::WorldUp)};
            registry.emplace<Core::Transform>(entity, transform);
            registry.emplace<Core::PreviousTransform>(entity, Core::PreviousTransform{.value = transform});

            const Dog& dog = registry.emplace<Dog>(entity);
            registry.emplace<Physics::CharacterBody>(entity, Physics::CharacterBody{.halfExtents = DogHalfExtents});
            registry.emplace<Health>(entity, Health{.current = dog.maximumHealth, .maximum = dog.maximumHealth});
            registry.emplace<StepSmoothing>(entity);
            registry.emplace<Renderer::DrawOffset>(entity);

            const Renderer::ModelHandle model = assets.LoadModel(DogModelPath, Core::AssetLifetime::Level);
            registry.emplace<Renderer::ModelRenderer>(entity, Renderer::ModelRenderer{
                .model = model,
                .shaderProgram = assets.shaders.Load("Shaders/TexturedShaded"),
            });

            // It stands idle until its mind tells it otherwise.
            Animator& animator = registry.emplace<Animator>(entity, Animator{
                .model = model,
                .segments = CreateClipSegments(assets.models.Get(model)),
            });
            const auto idle = std::ranges::find(animator.segments, "Idle", &AnimationSegment::name);
            if (idle != animator.segments.end())
                animator.current.segment = static_cast<std::size_t>(idle - animator.segments.begin());

            return entity;
        }
    }

    std::vector<entt::entity> SpawnMonsters(entt::registry& registry, Renderer::RenderAssets& assets,
                                            std::span<const World::MonsterStart> starts)
    {
        std::vector<entt::entity> monsters;
        for (const World::MonsterStart& start : starts)
        {
            if (start.className == "monster_dog")
                monsters.push_back(SpawnDog(registry, assets, start));
            else
                Core::Log::Write(LogCategory::World, LogLevel::Warning, "Unknown monster {} skipped", start.className);
        }
        return monsters;
    }

    void DestroyMonsters(entt::registry& registry, std::span<const entt::entity> monsters)
    {
        for (const entt::entity entity : monsters)
            if (registry.valid(entity))
                registry.destroy(entity);
    }

    void KillMonster(entt::registry& registry, entt::entity monster)
    {
        if (registry.valid(monster) && registry.all_of<Dog>(monster))
            registry.destroy(monster);
    }

    void UpdateMonsters(entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                        const Physics::PhysicsSettings& physicsSettings, const Physics::MovementSettings& movementSettings,
                        float tickDuration)
    {
        for (const auto [entity, dog, transform, body] : registry.view<Dog, Core::Transform, Physics::CharacterBody>().each())
        {
            // No command yet: it only slides after a push, slowed by friction, and falls when there is no floor. It
            // stops at the walls and at the other characters.
            const std::vector<World::CollisionBrush> obstacles = GatherCollisionBrushes(registry, brushes, entity);
            Physics::UpdateCharacter(body, transform, obstacles, physicsSettings, movementSettings, Physics::MoveCommand{},
                                     tickDuration);

            // Up a stair the body jumps up at once; the model glides after it.
            StepSmoothing& smoothing = registry.get<StepSmoothing>(entity);
            UpdateStepSmoothing(smoothing, body.steppedUpHeight, tickDuration);
            registry.replace<Renderer::DrawOffset>(entity, Renderer::DrawOffset{
                .offset = glm::vec3(0.0f, smoothing.offset, 0.0f),
                .previousOffset = glm::vec3(0.0f, smoothing.previousOffset, 0.0f),
            });
        }
    }
}
