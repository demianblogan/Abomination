#include "Gameplay/Enemies/Monsters.h"

#include "Core/Logging/Log.h"
#include "Core/Profiling/ProfileZone.h"
#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Animation/Animator.h"
#include "Gameplay/Characters/CharacterCollision.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/Enemies/Corpses.h"
#include "Gameplay/Enemies/GroundFit.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Navigation/NavMesh.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/DrawOffset.h"
#include "Renderer/ModelRenderer.h"
#include "World/CollisionTrace.h"
#include "World/MonsterStart.h"
#include "World/PlayerStart.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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
            PROFILE_ZONE();

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
            // the size of the dog (Tools/Blender/RigDog.py), drawn DogScale times bigger, with its middle at the middle of
            // the box.
            const glm::vec3 center = start.origin + glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
            const Core::Transform transform{
                .position = center,
                .rotation = glm::angleAxis(start.yaw, Core::WorldUp),
                .scale = glm::vec3(DogScale),
            };
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
                .shaderProgram = assets.shaders.Load("Shaders/Lit"),
            });

            // It stands idle until its mind tells it otherwise.
            Animator& animator = registry.emplace<Animator>(entity, Animator{
                .model = model,
                .segments = FindModelSegments(DogModelPath, assets.models.Get(model)),
            });
            animator.current.segment = FindSegment(animator, "Idle").value_or(0);

            // It dies once: its death clip stops on the last pose, the body lying on the floor.
            if (const std::optional<std::size_t> death = FindSegment(animator, "Death"); death.has_value())
                animator.segments[*death].isLooping = false;

            return entity;
        }

        // How often the path of a dog is found again: the player moves, the old path goes where they were.
        constexpr float RepathInterval = 0.2f;

        // A corner of the path closer than this (along the floor) is reached: the dog turns to the next one.
        constexpr float CornerReachDistance = 0.4f;

        // A path ending farther than this from where it should go does not get there.
        constexpr float UnreachableDistance = 1.0f;

        // A goal lower than this below the feet of the dog is under a ledge: the dog jumps down to it.
        constexpr float LedgeDrop = 0.6f;

        // A dog closer than this (along the floor) to the end of a path that cannot reach the player waits there: other
        // dogs crowding the same place keep it from getting much closer.
        constexpr float WaitDistance = 1.5f;

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
                dog.isPathShort = false;
                if (navMesh->IsStraightWayClear(feet, *goal))
                    dog.path.clear();
                else
                    dog.path = navMesh->FindPath(feet, *goal);

                // A path that ends far from the goal cannot reach it. A goal below (the player under a ledge the navmesh
                // does not lead down from): the dog runs straight at it instead, and jumps down, as without the
                // navmesh. A goal above (the player on the altar): it runs as close as it can, and waits there.
                if (!dog.path.empty() && glm::distance(dog.path.back(), *goal) > UnreachableDistance)
                {
                    if (goal->y < feet.y - LedgeDrop)
                        dog.path.clear();
                    else
                        dog.isPathShort = true;
                }
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

        // What the player is, for every dog this tick: where, whether they live, their eyes, and their weapon (shots).
        struct PlayerView
        {
            const Core::Transform& transform;
            bool isAlive = true;
            glm::vec3 eyes{0.0f};
            glm::vec3 feet{0.0f};
            const Weapon* weapon = nullptr;
        };

        // The dogs killed since the last tick (a shot took all their health) die. A blow that left enough over tears
        // the dog apart at once (gibs); otherwise a yelp where it was, its death clip, and it is a dog no more but a
        // body (see Corpses.h).
        void KillDogs(GameplayState& state, entt::registry& registry, Audio::AudioEngine& audio, const glm::vec3& shotFrom)
        {
            PROFILE_ZONE();

            std::vector<entt::entity> killed;
            for (const auto [entity, dog, health] : registry.view<const Dog, const Health>().each())
                if (health.current <= 0.0f)
                    killed.push_back(entity);

            for (const entt::entity entity : killed)
            {
                if (BurstIfTornApart(state, registry, audio, entity, shotFrom))
                    continue;

                audio.Play(state.dogSounds.death, registry.get<Core::Transform>(entity).position);
                registry.remove<Dog>(entity);
                LayDownCorpse(state, registry, entity);

                Animator& animator = registry.get<Animator>(entity);
                animator.speed = 1.0f;
                PlayAnimation(animator, "Death", 0.1f);
            }
        }

        // What the dog perceives of the player this tick: where they are and whether they live, a line of sight from its
        // eyes to theirs (through the level only: other characters do not hide the player), the shots of their weapon
        // since the last tick, its own health lost, and whether it was blocked.
        DogPerception PerceivePlayer(Dog& dog, const Core::Transform& transform, float health, const PlayerView& player,
                                     std::span<const World::CollisionBrush> sightBrushes, float gravity)
        {
            PROFILE_ZONE();

            DogPerception perception{
                .position = transform.position,
                .forward = transform.rotation * Core::LocalForward,
                .home = dog.home,
                .playerPosition = player.transform.position,
                .isPlayerAlive = player.isAlive,
                .wasHurt = health < dog.lastHealth,
                .isBlocked = dog.isBlocked,
                .gravity = gravity,
            };
            dog.lastHealth = health;

            // Its eyes are a little above the middle of its body.
            const glm::vec3 dogEyes = transform.position + glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y) * 0.6f, 0.0f);
            const World::TraceResult sight =
                World::TraceBox(sightBrushes, glm::dvec3(dogEyes), glm::dvec3(player.eyes), glm::dvec3(0.01));
            perception.hasLineOfSight = sight.fraction >= 1.0 && !sight.startsInSolid;

            if (player.weapon != nullptr && dog.lastShotCount < 0)
                dog.lastShotCount = player.weapon->shotCount;
            if (player.weapon != nullptr && player.weapon->shotCount != dog.lastShotCount)
            {
                perception.hasShotBeenFired = true;
                perception.shotPosition = player.weapon->lastShotStart;
                dog.lastShotCount = player.weapon->shotCount;
            }
            return perception;
        }

        // Its way around walls (feet to feet: the navmesh lies on the floor): towards the player in a chase, towards its
        // target on a patrol; whether it stands where it can get no closer; how much higher the player stands.
        void FindDogWay(Dog& dog, const glm::vec3& dogFeet, const PlayerView& player, const Navigation::NavMesh* navMesh,
                        float tickDuration, DogPerception& perception)
        {
            PROFILE_ZONE();

            std::optional<glm::vec3> goalFeet;
            if (dog.mind.state == DogState::Chase)
                goalFeet = player.feet;
            else if (dog.mind.state == DogState::Patrol)
                goalFeet = dog.mind.patrolTarget - glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
            perception.wayPoint = FindWayPoint(dog, dogFeet, goalFeet, navMesh, tickDuration);

            // At the end of a path that cannot reach the player: it can get no closer, and waits there.
            glm::vec2 toPathEnd(0.0f);
            if (!dog.path.empty())
                toPathEnd = glm::vec2(dog.path.back().x - dogFeet.x, dog.path.back().z - dogFeet.z);
            perception.cannotGetCloser = dog.isPathShort && glm::length(toPathEnd) < WaitDistance;
            perception.playerFeetAbove = player.feet.y - dogFeet.y;
        }

        // After the mind changed its state: a new chase or patrol finds its way at once, and a new patrol goes to a point
        // of the navmesh around where the dog appeared, one it can walk to (instead of anywhere around it, perhaps inside
        // a wall). The change is written to the log at Trace level (seen in the console with Trace checked).
        void OnDogStateChanged(Dog& dog, entt::entity entity, DogState stateBefore, const DogPerception& perception,
                               const DogSettings& settings, const Navigation::NavMesh* navMesh)
        {
            dog.path.clear();
            dog.repathTimer = 0.0f;

            if (dog.mind.state == DogState::Patrol && navMesh != nullptr)
            {
                const glm::vec3 homeFeet = dog.home - glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
                const std::optional<glm::vec3> target = navMesh->FindRandomPointAround(
                    homeFeet, settings.patrolRadius, [&dog] { return dog.random.GetFloat(0.0f, 1.0f); });
                if (target.has_value())
                    dog.mind.patrolTarget = *target + glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
            }

            Core::Log::Write(LogCategory::Gameplay, LogLevel::Trace, "Dog {}: {} -> {} (player {:.1f} m away{}{})",
                             static_cast<std::uint32_t>(entity), DogStateNames[static_cast<std::size_t>(stateBefore)],
                             DogStateNames[static_cast<std::size_t>(dog.mind.state)],
                             glm::length(perception.playerPosition - perception.position),
                             perception.hasLineOfSight ? ", seen" : "", perception.hasShotBeenFired ? ", shot heard" : "");
        }

        // It barks when it notices the player, then again and again after short random pauses while it chases them,
        // like a real dog. It yelps when hit (and still alive: the dead were taken before).
        void VoiceDog(GameplayState& state, Dog& dog, DogState stateBefore, const DogPerception& perception,
                      const glm::vec3& position, Audio::AudioEngine& audio, float tickDuration)
        {
            const DogSettings& settings = state.dogSettings;
            const bool noticesNow = dog.mind.state == DogState::Alert && stateBefore != DogState::Alert;
            dog.barkTimer -= tickDuration;
            if (noticesNow || (dog.mind.state == DogState::Chase && dog.barkTimer <= 0.0f))
            {
                audio.Play(state.dogSounds.bark, position);
                dog.barkTimer = dog.random.GetFloat(settings.barkIntervalMinimum, settings.barkIntervalMaximum);
            }
            if (perception.wasHurt)
                audio.Play(state.dogSounds.hurt, position);
        }

        // It turns to where it goes or looks, no faster than its turn speed.
        void TurnDog(Core::Transform& transform, const DogDecision& decision, float tickDuration)
        {
            if (glm::dot(decision.faceDirection, decision.faceDirection) <= 1e-6f)
                return;

            const glm::quat wanted = glm::quatLookAt(decision.faceDirection, Core::WorldUp);
            const float angle = glm::angle(wanted * glm::inverse(transform.rotation));
            const float step = decision.turnSpeed * tickDuration;
            transform.rotation = angle <= step ? wanted : glm::slerp(transform.rotation, wanted, step / angle);
        }

        // Moves the dog through the level as decided: a leap is a push on the body; walking and running go forward,
        // where it faces now (a dog never moves sideways, it turns and runs where it looks). Then it notices whether it
        // landed, whether it is blocked, and on a patrol it steps back from the edge of a drop.
        void MoveDog(GameplayState& state, entt::registry& registry, entt::entity entity, Dog& dog, Core::Transform& transform,
                     Physics::CharacterBody& body, const DogDecision& decision, std::span<const World::CollisionBrush> brushes,
                     Audio::AudioEngine& audio, float tickDuration)
        {
            PROFILE_ZONE();

            // A push up takes the body off the ground, like a jump: on the ground the movement would lay the push flat
            // along the floor.
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
            const glm::vec3 run = decision.speed > 0.0f ? glm::normalize(forward) * decision.speed : glm::vec3(0.0f);
            const Physics::MoveCommand command{
                .wishDirection = decision.speed > 0.0f ? glm::normalize(forward) : glm::vec3(0.0f),
            };

            // On the ground a dog goes where it faces at once: its velocity is set, not built up. Through acceleration
            // and friction (made for the player, who slides a little like in Quake) the velocity of the old direction
            // died away slowly after every turn, and a chasing dog drifted like on ice. The movement code below still
            // collides, steps and slopes it; friction and acceleration change it only by a hair now.
            if (body.isOnGround)
                body.velocity = glm::vec3(run.x, body.velocity.y, run.z);

            const glm::vec3 positionBefore = transform.position;
            const std::span<const World::CollisionBrush> characters =
                GatherCharacterBoxes(registry, entity, state.characterBoxMemory);
            const bool wasOnGround = body.isOnGround;
            Physics::UpdateCharacter(body, transform, World::CollisionWorld(brushes, characters), state.physicsSettings,
                                     movement, command, tickDuration);

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
        }

        // The model on the ground under its paws: the heights in front of and behind its middle, along where it faces,
        // looked for within two steps up and down. Two, not one: on a slope the box rests on one of its bottom edges and
        // hangs over the rest of it, so the ground under the far paws is up to 0.9 m below the box on a 45 degree clip
        // ramp; within one step it was not found, counted as level with the box, and the dog tilted the wrong way (nose
        // down going up). A paw over nothing counts as standing at the bottom of the box. In the air, or with the fit off,
        // the model goes back to the box, untilted. Up a stair the box jumps up at once; the model glides after it.
        void FitDogToGround(const DogSettings& settings, float stepHeight, entt::registry& registry, entt::entity entity,
                            const Core::Transform& transform, const Physics::CharacterBody& body,
                            std::span<const World::CollisionBrush> brushes, float tickDuration)
        {
            PROFILE_ZONE();

            const float bottom = transform.position.y - static_cast<float>(DogHalfExtents.y);
            glm::vec3 forward = transform.rotation * Core::LocalForward;
            forward.y = 0.0f;
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
        PROFILE_ZONE();

        const Core::Transform* playerTransform = registry.try_get<Core::Transform>(state.player);
        if (playerTransform == nullptr)
            return;
        const Health* playerHealth = registry.try_get<Health>(state.player);
        const PlayerView player{
            .transform = *playerTransform,
            .isAlive = playerHealth == nullptr || playerHealth->current > 0.0f,
            .eyes = playerTransform->position + glm::vec3(0.0f, PlayerEyeHeight, 0.0f),
            .feet = playerTransform->position - glm::vec3(0.0f, static_cast<float>(World::PlayerHalfExtents.y), 0.0f),
            .weapon = registry.try_get<Weapon>(state.player),
        };

        KillDogs(state, registry, audio, player.eyes);
        UpdateCorpses(state, registry, brushes, audio, player.eyes, tickDuration);

        for (const auto [entity, dog, transform, body] : registry.view<Dog, Core::Transform, Physics::CharacterBody>().each())
        {
            DogPerception perception = PerceivePlayer(dog, transform, registry.get<Health>(entity).current, player,
                                                      sightBrushes, state.physicsSettings.gravity);
            const glm::vec3 dogFeet = transform.position - glm::vec3(0.0f, static_cast<float>(DogHalfExtents.y), 0.0f);
            FindDogWay(dog, dogFeet, player, navMesh, tickDuration, perception);

            // A frozen dog (the Enemies window) stands where it is, decides nothing and leaves its animation alone, so
            // any clip can be picked, paused and scrubbed in the Animation window.
            const DogState stateBefore = dog.mind.state;
            const DogDecision decision = state.areMonstersFrozen
                                             ? DogDecision{}
                                             : UpdateDogMind(dog.mind, perception, state.dogSettings, tickDuration, dog.random);
            if (dog.mind.state != stateBefore)
                OnDogStateChanged(dog, entity, stateBefore, perception, state.dogSettings, navMesh);

            VoiceDog(state, dog, stateBefore, perception, transform.position, audio, tickDuration);
            TurnDog(transform, decision, tickDuration);
            MoveDog(state, registry, entity, dog, transform, body, decision, brushes, audio, tickDuration);

            // The clip of what it does, cross-fading from the one before, at the speed of the state.
            if (!state.areMonstersFrozen)
            {
                Animator& animator = registry.get<Animator>(entity);
                animator.speed = decision.animationSpeed;
                PlayAnimation(animator, decision.animation, 0.2f);
            }

            if (decision.bites)
            {
                audio.Play(state.dogSounds.bite, transform.position);
                DamagePlayer(state, registry, audio,
                             PlayerDamage{.amount = state.dogSettings.damage, .kind = DamageKind::Melee,
                                          .sourcePosition = transform.position});
            }

            FitDogToGround(state.dogSettings, state.movementSettings.stepHeight, registry, entity, transform, body, brushes,
                           tickDuration);
        }
    }
}
