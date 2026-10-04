#include "Gameplay/Enemies/Monsters.h"

#include "Core/Logging/Log.h"
#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Animation/Animator.h"
#include "Gameplay/Characters/CharacterCollision.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/Debug/DebugLines.h"
#include "Renderer/DrawOffset.h"
#include "Renderer/ModelRenderer.h"
#include "World/CollisionTrace.h"
#include "World/MonsterStart.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <string>

namespace Abomination::Gameplay
{
    namespace
    {
        using Core::LogCategory;
        using Core::LogLevel;

        const std::string DogModelPath = "Models/Enemies/Dog.glb";

        // A dog that hardly moves for this long while it wants to is blocked.
        constexpr float BlockedTime = 0.3f;

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

            // Every dog gets its own random numbers, seeded by its entity.
            const auto seed = static_cast<std::uint32_t>(entity) * 2654435761u;
            const Dog& dog = registry.emplace<Dog>(entity, Dog{.home = center, .random = Core::Random(seed)});
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

    void UpdateMonsters(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                        Audio::AudioEngine& audio, float tickDuration)
    {
        // What every dog can perceive of the player: where they are, whether they live, and the shots of their weapon.
        const Core::Transform* player = registry.try_get<Core::Transform>(state.player);
        if (player == nullptr)
            return;
        const Health* playerHealth = registry.try_get<Health>(state.player);
        const Weapon* weapon = registry.try_get<Weapon>(state.player);
        const glm::vec3 playerEyes = player->position + glm::vec3(0.0f, PlayerEyeHeight, 0.0f);

        for (const auto [entity, dog, transform, body] : registry.view<Dog, Core::Transform, Physics::CharacterBody>().each())
        {
            const float health = registry.get<Health>(entity).current;
            DogPerception perception{
                .position = transform.position,
                .forward = transform.rotation * Core::LocalForward,
                .home = dog.home,
                .playerPosition = player->position,
                .isPlayerAlive = playerHealth == nullptr || playerHealth->current > 0.0f,
                .wasHurt = health < dog.lastHealth,
                .isBlocked = dog.isBlocked,
            };
            dog.lastHealth = health;

            // Sight: a thin trace from its eyes (a little above the middle of its body) to the player's, through the
            // level only (other characters do not hide the player).
            const glm::vec3 dogEyes = transform.position + glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y) * 0.6f, 0.0f);
            const World::TraceResult sight = World::TraceBox(brushes, glm::dvec3(dogEyes), glm::dvec3(playerEyes),
                                                             glm::dvec3(0.01));
            perception.hasLineOfSight = sight.fraction >= 1.0 && !sight.startsInSolid;

            if (weapon != nullptr && dog.lastShotCount < 0)
                dog.lastShotCount = weapon->shotCount;
            if (weapon != nullptr && weapon->shotCount != dog.lastShotCount)
            {
                perception.hasShotBeenFired = true;
                perception.shotPosition = weapon->lastShotStart;
                dog.lastShotCount = weapon->shotCount;
            }

            // A frozen dog (the Enemies window) stands where it is and decides nothing.
            const DogDecision decision = state.areMonstersFrozen
                                             ? DogDecision{}
                                             : UpdateDogMind(dog.mind, perception, state.dogSettings, tickDuration, dog.random);

            // It turns to where it goes or looks, no faster than its turn speed.
            if (glm::dot(decision.faceDirection, decision.faceDirection) > 1e-6f)
            {
                const glm::quat wanted = glm::quatLookAt(decision.faceDirection, Core::WorldUp);
                const float angle = glm::angle(wanted * glm::inverse(transform.rotation));
                const float step = decision.turnSpeed * tickDuration;
                transform.rotation = angle <= step ? wanted : glm::slerp(transform.rotation, wanted, step / angle);
            }

            // The leap is a push on the body. Walking and running go forward, where it faces now: a dog never moves
            // sideways, it turns and runs where it looks.
            if (body.isOnGround)
                body.velocity += decision.impulse;
            Physics::MovementSettings movement = state.movementSettings;
            movement.maxSpeed = decision.speed;
            // Below stopSpeed (3.1 m/s) friction takes at least friction × stopSpeed = 12.5 m/s every second, while a
            // walk gains only groundAcceleration × its speed (13 m/s at 1.3 m/s, 10 at 1 m/s): friction ate almost every
            // step and a slow walk never got going. Lowered to the speed it wants, the walk gains 10 and loses 4 times its
            // speed per second: it reaches it in about a sixth of a second, and stops as fast.
            if (decision.speed > 0.0f)
                movement.stopSpeed = glm::min(movement.stopSpeed, decision.speed);
            glm::vec3 forward = transform.rotation * Core::LocalForward;
            forward.y = 0.0f;
            const Physics::MoveCommand command{
                .wishDirection = decision.speed > 0.0f ? glm::normalize(forward) : glm::vec3(0.0f),
            };
            const glm::vec3 positionBefore = transform.position;
            const std::vector<World::CollisionBrush> obstacles = GatherCollisionBrushes(registry, brushes, entity);
            Physics::UpdateCharacter(body, transform, obstacles, state.physicsSettings, movement, command, tickDuration);

            // Wanting to move but hardly moving (a quarter of the way it wanted) for a while: something is in the way.
            // Not after one tick: starting from a stop, the body speeds up over a few ticks, and its first steps are
            // short (that once stopped every patrol at its first step).
            const glm::vec3 moved = transform.position - positionBefore;
            const bool hardlyMoved = decision.speed > 0.0f &&
                                     glm::length(glm::vec2(moved.x, moved.z)) < decision.speed * tickDuration * 0.25f;
            dog.blockedTime = hardlyMoved ? dog.blockedTime + tickDuration : 0.0f;
            dog.isBlocked = dog.blockedTime >= BlockedTime;

            // The clip of what it does, cross-fading from the one before, at the speed of the state.
            Animator& animator = registry.get<Animator>(entity);
            animator.speed = decision.animationSpeed;
            const auto segment = std::ranges::find(animator.segments, decision.animation, &AnimationSegment::name);
            if (segment != animator.segments.end())
                PlayAnimation(animator, static_cast<std::size_t>(segment - animator.segments.begin()), 0.2f);

            if (decision.bites)
            {
                DamagePlayer(state, registry, audio,
                             PlayerDamage{.amount = state.dogSettings.damage, .kind = DamageKind::Melee,
                                          .sourcePosition = transform.position});
            }

            // Up a stair the body jumps up at once; the model glides after it.
            StepSmoothing& smoothing = registry.get<StepSmoothing>(entity);
            UpdateStepSmoothing(smoothing, body.steppedUpHeight, tickDuration);
            registry.replace<Renderer::DrawOffset>(entity, Renderer::DrawOffset{
                .offset = glm::vec3(0.0f, smoothing.offset, 0.0f),
                .previousOffset = glm::vec3(0.0f, smoothing.previousOffset, 0.0f),
            });
        }
    }

    void AddMonsterDebugLines(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                              Renderer::DebugLines& lines)
    {
        if (!state.areDogSensesVisible)
            return;

        const DogSettings& settings = state.dogSettings;
        constexpr int Segments = 48;
        constexpr float FullTurn = 2.0f * std::numbers::pi_v<float>;

        // A circle lying flat around center.
        const auto addCircle = [&](const glm::vec3& center, float radius, const glm::vec3& color)
        {
            for (int segment = 0; segment < Segments; ++segment)
            {
                const float a = FullTurn * static_cast<float>(segment) / Segments;
                const float b = FullTurn * static_cast<float>(segment + 1) / Segments;
                lines.AddLine(center + glm::vec3(std::cos(a), 0.0f, std::sin(a)) * radius,
                              center + glm::vec3(std::cos(b), 0.0f, std::sin(b)) * radius, color);
            }
        };

        for (const auto [entity, dog, transform] : registry.view<const Dog, const Core::Transform>().each())
        {
            const Core::Transform drawn = Core::CalculateDrawnTransform(registry, entity, interpolationFactor);

            // On the floor under it, a little above so the lines are not hidden in it.
            const float floorOffset = 0.02f - static_cast<float>(DogHalfExtents.y);
            const glm::vec3 floor = drawn.position + glm::vec3(0.0f, floorOffset, 0.0f);

            // The field of view: two edges from the dog to the range of its sight, and the arc between them.
            glm::vec3 forward = drawn.rotation * Core::LocalForward;
            forward.y = 0.0f;
            forward = glm::normalize(forward);
            const float facing = std::atan2(forward.z, forward.x);
            const float half = settings.fieldOfView * 0.5f;
            const glm::vec3 sightColor(1.0f, 0.9f, 0.2f);
            glm::vec3 previous{};
            for (int segment = 0; segment <= Segments; ++segment)
            {
                const float angle = facing - half + settings.fieldOfView * static_cast<float>(segment) / Segments;
                const glm::vec3 point = floor + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * settings.sightRange;
                if (segment == 0 || segment == Segments)
                    lines.AddLine(floor, point, sightColor);
                if (segment > 0)
                    lines.AddLine(previous, point, sightColor);
                previous = point;
            }

            addCircle(floor, settings.senseRadius, glm::vec3(1.0f, 0.5f, 0.1f));
            addCircle(floor, settings.hearingRange, glm::vec3(0.3f, 0.5f, 1.0f));

            // The patrol area stays where the dog appeared.
            addCircle(dog.home + glm::vec3(0.0f, floorOffset, 0.0f), settings.patrolRadius, glm::vec3(0.3f, 1.0f, 0.3f));
        }
    }
}
