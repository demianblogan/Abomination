#include "Gameplay/Enemies/Monsters.h"

#include "Core/Logging/Log.h"
#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Animation/Animator.h"
#include "Gameplay/Characters/CharacterCollision.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/Enemies/GroundFit.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Navigation/NavMesh.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/Debug/DebugLines.h"
#include "Renderer/DrawOffset.h"
#include "Renderer/ModelRenderer.h"
#include "World/CollisionTrace.h"
#include "World/MonsterStart.h"
#include "World/PlayerStart.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        using Core::LogCategory;
        using Core::LogLevel;

        const std::string DogModelPath = "Models/Enemies/Dog.glb";

        // A dog that hardly moves for this long while it wants to is blocked.
        constexpr float BlockedTime = 0.3f;

        // Half the size of the thin box traced down to find the ground under a point (1 cm).
        constexpr double ProbeHalfSize = 0.01;

        // The height of the ground under the point (x, z) of a body whose box stands at bottom: a thin box traced down
        // from reach above the bottom to reach below it. None if there is no ground within reach. A start inside a
        // brush means ground even higher (the next step but one of a stair, under the front of a long body): it counts
        // as ground at the start, reach above the bottom.
        std::optional<float> FindGround(std::span<const World::CollisionBrush> brushes, float x, float z, float bottom,
                                        float reach)
        {
            const glm::dvec3 start(x, bottom + reach, z);
            const glm::dvec3 end(x, bottom - reach, z);
            const World::TraceResult trace = World::TraceBox(brushes, start, end, glm::dvec3(ProbeHalfSize));
            if (trace.startsInSolid)
                return bottom + reach;
            if (trace.fraction >= 1.0)
                return std::nullopt;

            // The trace stopped with the center of the thin box just above the ground.
            return static_cast<float>(trace.endPosition.y - ProbeHalfSize);
        }

        // How many corners of a dog's box centered at position have no ground under them within two steps, like
        // SV_CheckBottom in Quake: a box hanging over a drop has corners in the air. Two steps, not one: the dog is
        // longer (1.06 m) than two steps of a steep stair are deep (0.5 m each), so on a stair its hind corners are two
        // steps below its bottom. The corners are taken a little inside the box, so a corner against a wall does not
        // start inside it.
        int CountCornersOverDrop(std::span<const World::CollisionBrush> brushes, const glm::vec3& position, float stepHeight)
        {
            const float bottom = position.y - static_cast<float>(DogHalfExtents.y);
            const float halfX = static_cast<float>(DogHalfExtents.x) - 0.02f;
            const float halfZ = static_cast<float>(DogHalfExtents.z) - 0.02f;
            int count = 0;
            for (const float signX : {-1.0f, 1.0f})
                for (const float signZ : {-1.0f, 1.0f})
                    if (!FindGround(brushes, position.x + signX * halfX, position.z + signZ * halfZ, bottom,
                                    2.0f * stepHeight))
                        ++count;
            return count;
        }

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
            registry.emplace<GroundFit>(entity);
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

            // It dies once: its death clip stops on the last pose, the body lying on the floor.
            const auto death = std::ranges::find(animator.segments, "Death", &AnimationSegment::name);
            if (death != animator.segments.end())
                death->isLooping = false;

            return entity;
        }

        // The bodies of the dead: they fall and slide (a shot pushes them) through the level, but not against

        // How often the path of a dog is found again: the player moves, the old path goes where they were.
        constexpr float RepathInterval = 0.2f;

        // A corner of the path closer than this (along the floor) is reached: the dog turns to the next one.
        constexpr float CornerReachDistance = 0.4f;

        // A path ending farther than this from where it should go does not get there.
        constexpr float UnreachableDistance = 1.0f;

        // Where the dog at feet runs to on its way to goal (feet to feet): the next corner of its path on the navmesh, or
        // none while the way is straight (or there is no navmesh or no goal). The path is found again every
        // RepathInterval.
        std::optional<glm::vec3> FindWayPoint(Dog& dog, const glm::vec3& feet, const std::optional<glm::vec3>& goal,
                                              const Navigation::NavMesh* navMesh, float tickDuration)
        {
            if (!goal.has_value() || navMesh == nullptr)
            {
                dog.path.clear();
                return std::nullopt;
            }

            dog.repathTimer -= tickDuration;
            if (dog.repathTimer <= 0.0f)
            {
                dog.repathTimer = RepathInterval;
                if (navMesh->IsStraightWayClear(feet, *goal))
                    dog.path.clear();
                else
                    dog.path = navMesh->FindPath(feet, *goal);

                // A path that ends far from the goal cannot reach it (the player is below a ledge the navmesh does not
                // lead down from): the dog runs straight at it instead, and jumps down, as without the navmesh.
                if (!dog.path.empty() && glm::distance(dog.path.back(), *goal) > UnreachableDistance)
                    dog.path.clear();
            }
            if (dog.path.size() < 2)
                return std::nullopt;

            // The corners it has reached are dropped: the first one left is where it was, the second where it goes.
            const auto alongFloor = [](const glm::vec3& a, const glm::vec3& b)
            {
                return glm::length(glm::vec2(a.x - b.x, a.z - b.z));
            };
            while (dog.path.size() > 2 && alongFloor(dog.path[1], feet) < CornerReachDistance)
                dog.path.erase(dog.path.begin());
            return dog.path[1];
        }
        // characters; when there are more than state.maximumCorpses, the oldest sink into the floor and are gone.
        // A dead dog lies flat: its box is lowered to half its height, the bottom where it was, so shots above the body
        // pass over it. Tells how far the middle of the box went down: the model is drawn that much higher, where it was.
        float LowerCorpse(entt::registry& registry, entt::entity entity)
        {
            Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(entity);
            const auto lowered = static_cast<float>(body.halfExtents.y * 0.5);
            body.halfExtents.y *= 0.5;
            registry.get<Core::Transform>(entity).position.y -= lowered;
            registry.get<Core::PreviousTransform>(entity).value.position.y -= lowered;
            return lowered;
        }

        // A body (alive a moment ago, or already lying) that took enough damage beyond death bursts into gibs, flying
        // away from shotFrom (the eyes of the player, who shot it), and is gone. Tells whether it burst.
        bool BurstIfTornApart(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, entt::entity entity,
                              const glm::vec3& shotFrom)
        {
            const Health* health = registry.try_get<Health>(entity);
            if (health == nullptr || health->overkill < state.gibs.settings.burstDamage)
                return false;

            const glm::vec3 center = registry.get<Core::Transform>(entity).position;
            const glm::vec3 halfExtents(registry.get<Physics::CharacterBody>(entity).halfExtents);
            BurstIntoGibs(state, registry, audio, center, halfExtents, center - shotFrom);
            registry.destroy(entity);
            return true;
        }

        void UpdateCorpses(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                           Audio::AudioEngine& audio, const glm::vec3& shotFrom, float tickDuration)
        {
            // Bodies shot to pieces burst.
            std::vector<entt::entity> corpseEntities;
            for (const auto [entity, corpse] : registry.view<const Corpse>().each())
                corpseEntities.push_back(entity);
            for (const entt::entity entity : corpseEntities)
                static_cast<void>(BurstIfTornApart(state, registry, audio, entity, shotFrom));

            // Too many bodies: the oldest ones that do not sink yet start to. A sinking body takes no more shots.
            std::vector<std::pair<std::uint64_t, entt::entity>> lying;
            for (const auto [entity, corpse] : registry.view<const Corpse>().each())
                if (!corpse.isSinking)
                    lying.emplace_back(corpse.order, entity);
            const auto excess = static_cast<std::ptrdiff_t>(lying.size()) - std::max(state.maximumCorpses, 0);
            if (excess > 0)
            {
                std::ranges::sort(lying);
                for (std::ptrdiff_t index = 0; index < excess; ++index)
                {
                    const entt::entity entity = lying[static_cast<std::size_t>(index)].second;
                    registry.get<Corpse>(entity).isSinking = true;
                    registry.remove<Health>(entity);
                }
            }

            // A body lies still: no wish to move, and friction stops a slide.
            Physics::MovementSettings movement = state.movementSettings;
            movement.maxSpeed = 0.0f;

            std::vector<entt::entity> gone;
            const auto corpses = registry.view<Corpse, Core::Transform, Physics::CharacterBody, GroundFit>();
            for (const auto [entity, corpse, transform, body, fit] : corpses.each())
            {
                Physics::UpdateCharacter(body, transform, brushes, state.physicsSettings, movement, {}, tickDuration);

                // Drawn where it lay down (the fit to the ground of its last moment alive), lower as it sinks; gone once
                // its whole body is under the floor.
                const float depthBefore = corpse.sunkDepth;
                if (corpse.isSinking)
                    corpse.sunkDepth += CorpseSinkSpeed * tickDuration;
                registry.replace<Renderer::DrawOffset>(entity, Renderer::DrawOffset{
                    .offset = glm::vec3(0.0f, fit.offset + corpse.raise - corpse.sunkDepth, 0.0f),
                    .previousOffset = glm::vec3(0.0f, fit.offset + corpse.raise - depthBefore, 0.0f),
                    .pitch = fit.pitch,
                    .previousPitch = fit.pitch,
                });
                if (corpse.sunkDepth > 2.0f * (static_cast<float>(body.halfExtents.y) + corpse.raise))
                    gone.push_back(entity);
            }
            registry.destroy(gone.begin(), gone.end());
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

    void UpdateMonsters(GameplayState& state, entt::registry& registry, std::span<const World::CollisionBrush> brushes,
                        std::span<const World::CollisionBrush> sightBrushes, const Navigation::NavMesh* navMesh,
                        Audio::AudioEngine& audio, float tickDuration)
    {
        // What every dog can perceive of the player: where they are, whether they live, and the shots of their weapon.
        const Core::Transform* player = registry.try_get<Core::Transform>(state.player);
        if (player == nullptr)
            return;
        const Health* playerHealth = registry.try_get<Health>(state.player);
        const Weapon* weapon = registry.try_get<Weapon>(state.player);
        const glm::vec3 playerEyes = player->position + glm::vec3(0.0f, PlayerEyeHeight, 0.0f);

        // The dogs killed since the last tick (a shot took all their health) die. A blow that left enough over tears
        // the dog apart at once (gibs); otherwise a yelp where it was, its death clip, and it is a dog no more but a
        // body (see Corpse).
        std::vector<entt::entity> killed;
        for (const auto [entity, dog, health] : registry.view<const Dog, const Health>().each())
            if (health.current <= 0.0f)
                killed.push_back(entity);
        for (const entt::entity entity : killed)
        {
            if (BurstIfTornApart(state, registry, audio, entity, playerEyes))
                continue;

            audio.Play(state.dogSounds.death, registry.get<Core::Transform>(entity).position);
            registry.remove<Dog>(entity);
            registry.emplace<Corpse>(entity, Corpse{.order = state.nextCorpseOrder++, .raise = LowerCorpse(registry, entity)});

            Animator& animator = registry.get<Animator>(entity);
            animator.speed = 1.0f;
            const auto death = std::ranges::find(animator.segments, "Death", &AnimationSegment::name);
            if (death != animator.segments.end())
                PlayAnimation(animator, static_cast<std::size_t>(death - animator.segments.begin()), 0.1f);
        }
        UpdateCorpses(state, registry, brushes, audio, playerEyes, tickDuration);

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
                .gravity = state.physicsSettings.gravity,
            };
            dog.lastHealth = health;

            // Sight: a thin trace from its eyes (a little above the middle of its body) to the player's, through the
            // level only (other characters do not hide the player).
            const glm::vec3 dogEyes = transform.position + glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y) * 0.6f, 0.0f);
            const World::TraceResult sight = World::TraceBox(sightBrushes, glm::dvec3(dogEyes), glm::dvec3(playerEyes),
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

            // Its way around walls: towards the player in a chase, towards its target on a patrol (feet to feet: the
            // navmesh lies on the floor).
            const glm::vec3 dogFeet = transform.position - glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
            std::optional<glm::vec3> goalFeet;
            if (dog.mind.state == DogState::Chase)
                goalFeet = player->position - glm::vec3(0.0f, static_cast<float>(World::PlayerHalfExtents.y), 0.0f);
            else if (dog.mind.state == DogState::Patrol)
                goalFeet = dog.mind.patrolTarget - glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
            perception.wayPoint = FindWayPoint(dog, dogFeet, goalFeet, navMesh, tickDuration);

            const DogState stateBefore = dog.mind.state;
            // A frozen dog (the Enemies window) stands where it is and decides nothing.
            const DogDecision decision = state.areMonstersFrozen
                                             ? DogDecision{}
                                             : UpdateDogMind(dog.mind, perception, state.dogSettings, tickDuration, dog.random);

            // A new patrol goes to a point of the navmesh around where the dog appeared, one it can walk to, instead of
            // anywhere around it (perhaps inside a wall). A new chase or patrol finds its way at once.
            if (dog.mind.state != stateBefore)
            {
                dog.path.clear();
                dog.repathTimer = 0.0f;
            }
            if (dog.mind.state == DogState::Patrol && stateBefore != DogState::Patrol && navMesh != nullptr)
            {
                const glm::vec3 homeFeet = dog.home - glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
                const std::optional<glm::vec3> target = navMesh->FindRandomPointAround(
                    homeFeet, state.dogSettings.patrolRadius, [&dog] { return dog.random.GetFloat(0.0f, 1.0f); });
                if (target.has_value())
                    dog.mind.patrolTarget = *target + glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
            }

            // What it decides, for the log at Trace level (seen in the console with Trace checked).
            if (dog.mind.state != stateBefore)
            {
                Core::Log::Write(LogCategory::Gameplay, LogLevel::Trace, "Dog {}: {} -> {} (player {:.1f} m away{}{})",
                                 static_cast<std::uint32_t>(entity), DogStateNames[static_cast<std::size_t>(stateBefore)],
                                 DogStateNames[static_cast<std::size_t>(dog.mind.state)],
                                 glm::length(perception.playerPosition - perception.position),
                                 perception.hasLineOfSight ? ", seen" : "", perception.hasShotBeenFired ? ", shot heard" : "");
            }

            // It barks when it notices the player, then again and again after short random pauses while it chases
            // them, like a real dog. It yelps when hit (and still alive: the dead were taken above).
            const DogSettings& dogSettings = state.dogSettings;
            const bool barksNow = dog.mind.state == DogState::Alert && stateBefore != DogState::Alert;
            dog.barkTimer -= tickDuration;
            if (barksNow || (dog.mind.state == DogState::Chase && dog.barkTimer <= 0.0f))
            {
                audio.Play(state.dogSounds.bark, transform.position);
                dog.barkTimer = dog.random.GetFloat(dogSettings.barkIntervalMinimum, dogSettings.barkIntervalMaximum);
            }
            if (perception.wasHurt)
                audio.Play(state.dogSounds.hurt, transform.position);

            // It turns to where it goes or looks, no faster than its turn speed.
            if (glm::dot(decision.faceDirection, decision.faceDirection) > 1e-6f)
            {
                const glm::quat wanted = glm::quatLookAt(decision.faceDirection, Core::WorldUp);
                const float angle = glm::angle(wanted * glm::inverse(transform.rotation));
                const float step = decision.turnSpeed * tickDuration;
                transform.rotation = angle <= step ? wanted : glm::slerp(transform.rotation, wanted, step / angle);
            }

            // The leap is a push on the body. Walking and running go forward, where it faces now: a dog never moves
            // sideways, it turns and runs where it looks. A push up takes the body off the ground, like a jump: on the
            // ground the movement would lay the push flat along the floor.
            if (body.isOnGround && glm::dot(decision.impulse, decision.impulse) > 0.0f)
            {
                body.velocity += decision.impulse;
                body.isOnGround = decision.impulse.y <= 0.0f;
            }
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

            // On the ground a dog goes where it faces at once: its velocity is set, not built up. Through acceleration
            // and friction (made for the player, who slides a little like in Quake) the velocity of the old direction
            // died away slowly after every turn, and a chasing dog drifted like on ice. The movement code below still
            // collides, steps and slopes it; friction and acceleration change it only by a hair now.
            if (body.isOnGround)
            {
                const glm::vec3 run = decision.speed > 0.0f ? glm::normalize(forward) * decision.speed : glm::vec3(0.0f);
                body.velocity = glm::vec3(run.x, body.velocity.y, run.z);
            }
            const glm::vec3 positionBefore = transform.position;
            const std::vector<World::CollisionBrush> obstacles = GatherCollisionBrushes(registry, brushes, entity);
            const bool wasOnGround = body.isOnGround;
            Physics::UpdateCharacter(body, transform, obstacles, state.physicsSettings, movement, command, tickDuration);

            // The paws hit the ground at the end of a leap.
            if (!wasOnGround && body.isOnGround && dog.mind.state == DogState::Leap && dog.mind.hasLeapt)
                audio.Play(state.dogSounds.land, transform.position);

            // Wanting to move but hardly moving (a quarter of the way it wanted) for a while: something is in the way.
            // Not after one tick: starting from a stop, the body speeds up over a few ticks, and its first steps are
            // short (that once stopped every patrol at its first step).
            const glm::vec3 moved = transform.position - positionBefore;
            const bool hardlyMoved = decision.speed > 0.0f &&
                                     glm::length(glm::vec2(moved.x, moved.z)) < decision.speed * tickDuration * 0.25f;
            dog.blockedTime = hardlyMoved ? dog.blockedTime + tickDuration : 0.0f;
            dog.isBlocked = dog.blockedTime >= BlockedTime;

            // A patrolling dog does not walk over the edge of a drop: a step that leaves more corners of its box in the
            // air than before is taken back, and the dog is blocked at once (it stands, then walks elsewhere). Fewer or
            // as many is allowed, so a dog placed hanging over an edge can still walk off it.
            const float stepHeight = state.movementSettings.stepHeight;
            if (state.dogSettings.avoidsLedges && dog.mind.state == DogState::Patrol && wasOnGround &&
                CountCornersOverDrop(brushes, transform.position, stepHeight) >
                    CountCornersOverDrop(brushes, positionBefore, stepHeight))
            {
                transform.position = positionBefore;
                body.velocity = glm::vec3(0.0f, body.velocity.y, 0.0f);
                body.isOnGround = true;
                body.steppedUpHeight = 0.0f;
                dog.blockedTime = BlockedTime;
                dog.isBlocked = true;
            }

            // The clip of what it does, cross-fading from the one before, at the speed of the state. A frozen dog leaves
            // its animation alone, so any clip can be picked, paused and scrubbed in the Animation window.
            if (!state.areMonstersFrozen)
            {
                Animator& animator = registry.get<Animator>(entity);
                animator.speed = decision.animationSpeed;
                const auto segment = std::ranges::find(animator.segments, decision.animation, &AnimationSegment::name);
                if (segment != animator.segments.end())
                    PlayAnimation(animator, static_cast<std::size_t>(segment - animator.segments.begin()), 0.2f);
            }

            if (decision.bites)
            {
                audio.Play(state.dogSounds.bite, transform.position);
                DamagePlayer(state, registry, audio,
                             PlayerDamage{.amount = state.dogSettings.damage, .kind = DamageKind::Melee,
                                          .sourcePosition = transform.position});
            }

            // The model on the ground under its paws: the heights in front of and behind its middle, along where it
            // faces, looked for within two steps up and down. Two, not one: on a slope the box rests on one of its bottom
            // edges and hangs over the rest of it, so the ground under the far paws is up to 0.9 m below the box on a
            // 45 degree clip ramp; within one step it was not found, counted as level with the box, and the dog tilted
            // the wrong way (nose down going up). A paw over nothing counts as standing at the bottom of the box. In the
            // air, or with the fit off, the model goes back to the box, untilted.
            const DogSettings& settings = state.dogSettings;
            const float bottom = transform.position.y - static_cast<float>(DogHalfExtents.y);
            GroundFitTarget target{.offset = 0.0f, .pitch = 0.0f};
            if (settings.fitsToGround && body.isOnGround && glm::dot(forward, forward) > 1e-6f)
            {
                const glm::vec3 pawOffset = glm::normalize(forward) * settings.pawDistance;
                const glm::vec3 front = transform.position + pawOffset;
                const glm::vec3 back = transform.position - pawOffset;
                const float frontGround = FindGround(brushes, front.x, front.z, bottom, 2.0f * stepHeight).value_or(bottom);
                const float backGround = FindGround(brushes, back.x, back.z, bottom, 2.0f * stepHeight).value_or(bottom);
                target = CalculateGroundFitTarget(frontGround, backGround, bottom, settings.pawDistance,
                                                  settings.maximumTilt, stepHeight);
            }

            // Up a stair the box jumps up at once; the model glides after it.
            GroundFit& fit = registry.get<GroundFit>(entity);
            UpdateGroundFit(fit, target, body.steppedUpHeight, settings.heightFollowSpeed, settings.tiltFollowSpeed,
                            MaximumStepLag, tickDuration);
            registry.replace<Renderer::DrawOffset>(entity, Renderer::DrawOffset{
                .offset = glm::vec3(0.0f, fit.offset, 0.0f),
                .previousOffset = glm::vec3(0.0f, fit.previousOffset, 0.0f),
                .pitch = fit.pitch,
                .previousPitch = fit.previousPitch,
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

            // Its way around walls: from where it is through the corners of its path (white).
            if (dog.path.size() >= 2)
            {
                const glm::vec3 lift(0.0f, 0.06f, 0.0f);
                const glm::vec3 pathColor(1.0f, 1.0f, 1.0f);
                lines.AddLine(floor + lift, dog.path[1] + lift, pathColor);
                for (std::size_t corner = 2; corner < dog.path.size(); ++corner)
                    lines.AddLine(dog.path[corner - 1] + lift, dog.path[corner] + lift, pathColor);
            }

            addCircle(floor, settings.senseRadius, glm::vec3(1.0f, 0.5f, 0.1f));
            addCircle(floor, settings.hearingRange, glm::vec3(0.3f, 0.5f, 1.0f));

            // The patrol area stays where the dog appeared.
            addCircle(dog.home + glm::vec3(0.0f, floorOffset, 0.0f), settings.patrolRadius, glm::vec3(0.3f, 1.0f, 0.3f));
        }
    }

    void AddNavMeshDebugLines(const GameplayState& state, const Navigation::NavMesh* navMesh, Renderer::DebugLines& lines)
    {
        if (!state.isNavMeshVisible || navMesh == nullptr)
            return;

        // A little above the floor, so the lines are not hidden in it.
        constexpr glm::vec3 Lift{0.0f, 0.04f, 0.0f};
        constexpr glm::vec3 Color{0.3f, 0.95f, 0.55f};
        for (const Navigation::NavMeshPolygon& polygon : navMesh->GetPolygons())
            for (std::size_t corner = 0; corner < polygon.size(); ++corner)
                lines.AddLine(polygon[corner] + Lift, polygon[(corner + 1) % polygon.size()] + Lift, Color);
    }
}
