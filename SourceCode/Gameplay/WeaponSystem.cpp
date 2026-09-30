#include "Gameplay/WeaponSystem.h"

#include "Audio/AudioEngine.h"
#include "Core/Math/BoundingBox.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/Effects.h"
#include "Gameplay/Health.h"
#include "Gameplay/MouseLook.h"
#include "Gameplay/Player.h"
#include "Gameplay/TargetDummy.h"
#include "Gameplay/ViewModel.h"
#include "Gameplay/ViewRecoil.h"
#include "Gameplay/Weapon.h"
#include "Input/ActionStates.h"
#include "Physics/CharacterBody.h"
#include "Renderer/Debug/DebugLines.h"
#include "World/CollisionTrace.h"

#include <glm/vec3.hpp>

#include <algorithm>
#include <numbers>
#include <optional>
#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        // How long the debug lines of a shot stay (seconds).
        constexpr float ShotLineDuration = 2.0f;

        // Half the size of the box drawn where a pellet hit (meters).
        constexpr float HitMarkerHalfSize = 0.03f;

        constexpr glm::vec3 PelletHitColor{1.0f, 0.3f, 0.2f};
        constexpr glm::vec3 PelletEntityHitColor{1.0f, 0.9f, 0.2f};
        constexpr glm::vec3 PelletMissColor{0.6f, 0.6f, 0.6f};

        struct HitEntity
        {
            entt::entity entity = entt::null;
            double distance = 0.0;
        };

        // The closest entity with Health and a CharacterBody (its box) that the ray enters before maxDistance, except the
        // shooter.
        HitEntity FindHitEntity(const entt::registry& registry, entt::entity shooter, const glm::dvec3& origin,
                                const glm::dvec3& direction, double maxDistance)
        {
            HitEntity closest;
            double closestDistance = maxDistance;
            for (const auto [entity, health, body, transform] :
                 registry.view<const Health, const Physics::CharacterBody, const Core::Transform>().each())
            {
                if (entity == shooter)
                    continue;

                const glm::dvec3 center(transform.position);
                const Core::BoundingBox box{.minimum = center - body.halfExtents, .maximum = center + body.halfExtents};
                const std::optional<double> distance = Core::IntersectRay(box, origin, direction, closestDistance);
                if (distance.has_value())
                {
                    closest = {.entity = entity, .distance = *distance};
                    closestDistance = *distance;
                }
            }

            return closest;
        }
    }

    void CollectWeaponInput(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                            bool canShoot)
    {
        Weapon* weapon = registry.try_get<Weapon>(state.player);
        if (weapon != nullptr && canShoot && actions.WasActionStarted(Input::Action::Fire))
            weapon->isFireRequested = true;
    }

    bool UpdateWeapon(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions, bool canShoot,
                      std::span<const World::CollisionBrush> brushes, Audio::AudioEngine& audio, float tickDuration)
    {
        Weapon* weapon = registry.try_get<Weapon>(state.player);
        if (weapon == nullptr)
            return false;

        weapon->cooldown -= tickDuration;
        weapon->secondsSinceLastShot += tickDuration;

        // Holding Fire keeps shooting at the rhythm of the weapon; a press between two ticks counts too.
        const bool wantsToFire = canShoot && (actions.IsActionActive(Input::Action::Fire) || weapon->isFireRequested);
        weapon->isFireRequested = false;
        if (!wantsToFire || weapon->cooldown > 0.0f)
            return false;

        // The wait restarts from the moment the weapon became ready, not from now: at 60 ticks per second the rhythm
        // stays exactly timeBetweenShots instead of drifting by up to a tick with every shot. It never goes below 0,
        // so a long pause does not store up shots.
        weapon->cooldown = std::max(weapon->cooldown, 0.0f) + weapon->settings.timeBetweenShots;

        // The eyes as they are in this tick (not interpolated: the simulation shoots from where the player really is).
        const Core::Transform eyes = CalculatePlayerEyeTransform(registry.get<Core::Transform>(state.player),
                                                                 registry.get<LookAngles>(state.player));
        const glm::vec3 forward = eyes.rotation * Core::LocalForward;
        const glm::vec3 up = eyes.rotation * Core::WorldUp;

        const std::vector<glm::vec3> directions = GeneratePelletDirections(
            forward, up, weapon->settings.spreadAngle, weapon->settings.pelletCount, weapon->random);

        weapon->lastShotStart = eyes.position;
        weapon->lastShotPellets.clear();
        weapon->secondsSinceLastShot = 0.0f;

        bool hasHurt = false;
        bool hasKilled = false;
        for (const glm::vec3& direction : directions)
        {
            // A pellet is a ray: a box of size 0 traced through the brushes. How far it flies before a wall stops it:
            const glm::dvec3 start(eyes.position);
            const double range = static_cast<double>(weapon->settings.range);
            const glm::dvec3 end = start + glm::dvec3(direction) * range;
            const World::TraceResult trace = World::TraceBox(brushes, start, end, glm::dvec3(0.0));
            const double wallDistance = trace.fraction * range;

            // The closest entity with health in front of that wall: its box is where the ray enters it.
            const HitEntity hit = FindHitEntity(registry, state.player, start, glm::dvec3(direction), wallDistance);
            if (hit.entity == entt::null)
            {
                const bool hasHitWall = trace.fraction < 1.0;
                weapon->lastShotPellets.push_back(PelletTrace{.end = glm::vec3(trace.endPosition), .hasHit = hasHitWall});

                // Sparks, dust and a mark where the pellet hit the wall.
                if (hasHitWall)
                    SpawnWallImpact(state.effects, glm::vec3(trace.endPosition), glm::vec3(trace.hitNormal));
                continue;
            }

            const glm::vec3 hitPoint(start + glm::dvec3(direction) * hit.distance);
            weapon->lastShotPellets.push_back(PelletTrace{.end = hitPoint, .hasHit = true, .hasHitEntity = true});
            SpawnBloodImpact(state.effects, hitPoint, direction);

            // The damage, and the push along the pellet, if the entity can move.
            hasHurt = true;
            if (ApplyDamage(registry.get<Health>(hit.entity), weapon->settings.damagePerPellet))
            {
                hasKilled = true;
                DestroyTargetDummy(registry, hit.entity);
            }
            else if (Physics::CharacterBody* body = registry.try_get<Physics::CharacterBody>(hit.entity); body != nullptr)
            {
                body->velocity += direction * weapon->settings.knockbackPerPellet;
            }
        }

        audio.Play(weapon->fireSound);

        // One confirmation per shot, however many pellets hit: a kill sounds different from a hit.
        if (hasKilled)
        {
            ++weapon->killCount;
            audio.Play(weapon->killSound);
        }
        else if (hasHurt)
        {
            ++weapon->hitCount;
            audio.Play(weapon->hitSound);
        }

        // The recoil: the view and the weapon in the hands jerk, and their springs bring them back.
        if (ViewRecoil* viewRecoil = registry.try_get<ViewRecoil>(state.player); viewRecoil != nullptr)
            KickViewRecoil(*viewRecoil);
        if (ViewModel* viewModel = registry.try_get<ViewModel>(state.player); viewModel != nullptr)
        {
            KickViewModelRecoil(viewModel->motion, viewModel->motionSettings);

            // The flash at the muzzle of the weapon in the hands, turned differently every shot.
            viewModel->flashTimeLeft = state.effects.settings.flashDuration;
            viewModel->flashRotation = state.effects.random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>);

            // The smoke goes into the world, from where the muzzle is seen: the muzzle relative to the eyes, placed by
            // the eyes. (The weapon is drawn with its own field of view, so this is close to, not exactly, where the
            // muzzle appears on the screen; for drifting smoke that does not show.)
            const glm::vec3 muzzle = eyes.position + eyes.rotation * CalculateViewModelMuzzle(*viewModel);
            SpawnMuzzleSmoke(state.effects, muzzle, forward);
        }

        return true;
    }

    void AddWeaponDebugLines(const GameplayState& state, const entt::registry& registry, Renderer::DebugLines& debugLines)
    {
        const Weapon* weapon = registry.try_get<Weapon>(state.player);
        if (weapon == nullptr || !weapon->areShotLinesVisible || weapon->secondsSinceLastShot > ShotLineDuration)
            return;

        for (const PelletTrace& pellet : weapon->lastShotPellets)
        {
            // Yellow on an entity, red on a wall, gray into nothing.
            const glm::vec3 color =
                pellet.hasHitEntity ? PelletEntityHitColor : (pellet.hasHit ? PelletHitColor : PelletMissColor);
            debugLines.AddLine(weapon->lastShotStart, pellet.end, color);
            if (pellet.hasHit)
                debugLines.AddBox(pellet.end - glm::vec3(HitMarkerHalfSize), pellet.end + glm::vec3(HitMarkerHalfSize), color);
        }
    }
}
