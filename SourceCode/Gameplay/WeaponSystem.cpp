#include "Gameplay/WeaponSystem.h"

#include "Audio/AudioEngine.h"
#include "Core/Scene/Transform.h"
#include "Gameplay/MouseLook.h"
#include "Gameplay/Player.h"
#include "Gameplay/Weapon.h"
#include "Input/ActionStates.h"
#include "Renderer/Debug/DebugLines.h"
#include "World/CollisionTrace.h"

#include <glm/vec3.hpp>

#include <algorithm>
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
        constexpr glm::vec3 PelletMissColor{0.6f, 0.6f, 0.6f};
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
        for (const glm::vec3& direction : directions)
        {
            // A pellet is a ray: a box of size 0 traced through the brushes.
            const glm::dvec3 start(eyes.position);
            const glm::dvec3 end = start + glm::dvec3(direction) * static_cast<double>(weapon->settings.range);
            const World::TraceResult trace = World::TraceBox(brushes, start, end, glm::dvec3(0.0));
            weapon->lastShotPellets.push_back(PelletTrace{.end = glm::vec3(trace.endPosition), .hasHit = trace.fraction < 1.0});
        }

        audio.Play(weapon->fireSound);

        return true;
    }

    void AddWeaponDebugLines(const GameplayState& state, const entt::registry& registry, Renderer::DebugLines& debugLines)
    {
        const Weapon* weapon = registry.try_get<Weapon>(state.player);
        if (weapon == nullptr || weapon->secondsSinceLastShot > ShotLineDuration)
            return;

        for (const PelletTrace& pellet : weapon->lastShotPellets)
        {
            const glm::vec3 color = pellet.hasHit ? PelletHitColor : PelletMissColor;
            debugLines.AddLine(weapon->lastShotStart, pellet.end, color);
            if (pellet.hasHit)
                debugLines.AddBox(pellet.end - glm::vec3(HitMarkerHalfSize), pellet.end + glm::vec3(HitMarkerHalfSize), color);
        }
    }
}
