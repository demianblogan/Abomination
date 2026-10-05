#include "Gameplay/Weapons/WeaponSystem.h"

#include "Audio/AudioEngine.h"
#include "Core/Math/BoundingBox.h"
#include "Core/Profiling/ProfileZone.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/Camera/MouseLook.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/Effects/Effects.h"
#include "Gameplay/Enemies/Monsters.h"
#include "Gameplay/Player/Player.h"
#include "Gameplay/Weapons/ViewRecoil.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
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

        // What the pellets of one shot did.
        struct ShotResult
        {
            bool hasHurt = false;
            bool hasKilled = false;
        };

        // Sends one pellet from start along direction: to the closest of the wall behind it and the characters in front
        // of that wall. It damages and pushes what it hits, throws the effects of the hit, and is recorded for the debug
        // lines.
        void FirePellet(GameplayState& state, entt::registry& registry, Weapon& weapon, const glm::dvec3& start,
                        const glm::vec3& direction, std::span<const World::CollisionBrush> brushes, ShotResult& result)
        {
            // A pellet is a ray: a box of size 0 traced through the brushes. How far it flies before a wall stops it:
            const double range = static_cast<double>(weapon.settings.range);
            const glm::dvec3 end = start + glm::dvec3(direction) * range;
            const World::TraceResult trace = World::TraceBox(brushes, start, end, glm::dvec3(0.0));
            const double wallDistance = trace.fraction * range;

            // The closest entity with health in front of that wall: its box is where the ray enters it.
            const HitEntity hit = FindHitEntity(registry, state.player, start, glm::dvec3(direction), wallDistance);
            if (hit.entity == entt::null)
            {
                const bool hasHitWall = trace.fraction < 1.0;
                weapon.lastShotPellets.push_back(PelletTrace{.end = glm::vec3(trace.endPosition), .hasHit = hasHitWall});

                // Sparks, dust and a mark where the pellet hit the wall.
                if (hasHitWall)
                    SpawnWallImpact(state.effects, glm::vec3(trace.endPosition), glm::vec3(trace.hitNormal));
                return;
            }

            const glm::vec3 hitPoint(start + glm::dvec3(direction) * hit.distance);
            weapon.lastShotPellets.push_back(PelletTrace{.end = hitPoint, .hasHit = true, .hasHitEntity = true});
            SpawnBloodImpact(state.effects, hitPoint, direction);

            // The damage, and the push along the pellet, if the entity can move.
            result.hasHurt = true;
            // A killed monster dies in its own update (see UpdateMonsters).
            if (ApplyDamage(registry.get<Health>(hit.entity), weapon.settings.damagePerPellet))
            {
                result.hasKilled = true;
            }
            else if (Physics::CharacterBody* body = registry.try_get<Physics::CharacterBody>(hit.entity); body != nullptr)
            {
                body->velocity += direction * weapon.settings.knockbackPerPellet;
            }
        }

        // Sends every pellet of a shot from the eyes, spread in the cone of the weapon.
        ShotResult FirePellets(GameplayState& state, entt::registry& registry, Weapon& weapon, const Core::Transform& eyes,
                               std::span<const World::CollisionBrush> brushes)
        {
            const glm::vec3 forward = eyes.rotation * Core::LocalForward;
            const glm::vec3 up = eyes.rotation * Core::WorldUp;
            const std::vector<glm::vec3> directions =
                GeneratePelletDirections(forward, up, weapon.settings.spreadAngle, weapon.settings.pelletCount, weapon.random);

            weapon.lastShotStart = eyes.position;
            weapon.lastShotPellets.clear();
            weapon.secondsSinceLastShot = 0.0f;

            ShotResult result;
            for (const glm::vec3& direction : directions)
                FirePellet(state, registry, weapon, glm::dvec3(eyes.position), direction, brushes, result);

            return result;
        }

        // What the player hears, sees and feels of a shot: its sound and the confirmation of a hit or a kill, the recoil
        // of the view and of the weapon in the hands, the muzzle flash and the smoke.
        void PlayShotFeedback(GameplayState& state, entt::registry& registry, Weapon& weapon, Audio::AudioEngine& audio,
                              const Core::Transform& eyes, const ShotResult& result)
        {
            audio.Play(weapon.fireSound);
            ++weapon.shotCount;

            // One confirmation per shot, however many pellets hit: a kill sounds different from a hit.
            if (result.hasKilled)
            {
                ++weapon.killCount;
                audio.Play(weapon.killSound);
            }
            else if (result.hasHurt)
            {
                ++weapon.hitCount;
                audio.Play(weapon.hitSound);
            }

            // The recoil: the view and the weapon in the hands jerk, and their springs bring them back.
            if (ViewRecoil* viewRecoil = registry.try_get<ViewRecoil>(state.player); viewRecoil != nullptr)
                KickViewRecoil(*viewRecoil);

            WeaponViewModel* weaponViewModel = registry.try_get<WeaponViewModel>(state.player);
            if (weaponViewModel == nullptr)
                return;

            KickWeaponViewModelRecoil(weaponViewModel->motion, weaponViewModel->motionSettings);

            // The pump starts its movement a moment after the shot (see PumpAction).
            weaponViewModel->pump.secondsSinceShot = 0.0f;

            // The flash at the muzzle of the weapon in the hands, turned differently every shot.
            weaponViewModel->flashTimeLeft = state.effects.settings.flashDuration;
            weaponViewModel->flashRotation = state.effects.random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>);

            // The smoke goes into the world, from where the muzzle is seen: the muzzle relative to the eyes, placed by the
            // eyes. (The weapon is drawn with its own field of view, so this is close to, not exactly, where the muzzle
            // appears on the screen; for drifting smoke that does not show.)
            const glm::vec3 muzzle = eyes.position + eyes.rotation * CalculateWeaponViewModelMuzzle(*weaponViewModel);
            SpawnMuzzleSmoke(state.effects, muzzle, eyes.rotation * Core::LocalForward);
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
        PROFILE_ZONE();

        Weapon* weapon = registry.try_get<Weapon>(state.player);
        if (weapon == nullptr)
            return false;

        weapon->cooldown -= tickDuration;
        weapon->secondsSinceLastShot += tickDuration;

        // Holding Fire keeps shooting at the rhythm of the weapon; a press between two ticks counts too.
        const bool wasFirePressed = weapon->isFireRequested;
        const bool wantsToFire = canShoot && (actions.IsActionActive(Input::Action::Fire) || wasFirePressed);
        weapon->isFireRequested = false;
        if (!wantsToFire || weapon->cooldown > 0.0f)
            return false;

        // No shot without ammunition (an entity without an Ammo component, like a test, shoots without counting). A new
        // press clicks, so the player knows why nothing happens; holding Fire clicks only once, not at every tick.
        if (Ammo* ammo = registry.try_get<Ammo>(state.player);
            ammo != nullptr && !TryUseAmmo(*ammo, weapon->settings.ammoType, weapon->settings.ammoPerShot))
        {
            if (wasFirePressed)
            {
                audio.Play(weapon->emptySound);
                ++weapon->emptyClickCount;
            }
            return false;
        }

        // The wait restarts from the moment the weapon became ready, not from now: at 60 ticks per second the rhythm
        // stays exactly the shot cycle instead of drifting by up to a tick with every shot. It never goes below 0,
        // so a long pause does not store up shots.
        weapon->cooldown = std::max(weapon->cooldown, 0.0f) + CalculateShotCycleDuration(weapon->settings.pumpAction);

        // The eyes as they are in this tick (not interpolated: the simulation shoots from where the player really is).
        const Core::Transform eyes = CalculatePlayerEyeTransform(registry.get<Core::Transform>(state.player),
                                                                 registry.get<LookAngles>(state.player));

        const ShotResult result = FirePellets(state, registry, *weapon, eyes, brushes);
        PlayShotFeedback(state, registry, *weapon, audio, eyes, result);

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
