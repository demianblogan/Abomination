#pragma once

#include "Audio/SoundEvent.h"
#include "Core/Math/Random.h"
#include "Gameplay/Weapons/Crosshair.h"

#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <vector>

namespace Abomination::Gameplay
{
    // Values a designer tunes (in the Weapon window of the debug overlay; later in the weapon configuration files, 0.6).
    struct WeaponSettings
    {
        // A shotgun shot is several pellets, each a straight line (hitscan: it hits at once, there is no flying
        // projectile), spread in a cone around where the player looks.
        int pelletCount = 6;

        // Half the angle of the cone (radians): a pellet flies at most this far off the middle of the screen. 4 degrees
        // spread the pellets about 70 cm wide at 10 m.
        float spreadAngle = glm::radians(4.0f);

        // Pellets fly this far (meters) and hit nothing beyond it.
        float range = 100.0f;

        // Seconds from one shot to the next, the pump-action rhythm.
        float timeBetweenShots = 0.8f;

        // What one pellet does to what it hits: the damage it takes from its Health, and how hard it pushes it away
        // (meters per second added to its velocity along the pellet). All six pellets at close range: 60 damage.
        float damagePerPellet = 10.0f;
        float knockbackPerPellet = 0.8f;
    };

    // One pellet of the last shot, for the debug lines.
    struct PelletTrace
    {
        glm::vec3 end{0.0f};

        // It hit a wall, or an entity with Health (then hasHit is true too).
        bool hasHit = false;
        bool hasHitEntity = false;
    };

    // Component of the player: the weapon they shoot with. The weapon in their hands as they see it is the WeaponViewModel;
    // this is what it does.
    struct Weapon
    {
        WeaponSettings settings;

        // How its crosshair and hit markers look.
        CrosshairSettings crosshair;

        Audio::SoundEvent fireSound;

        // Played once per shot that hurt something, and once per shot that killed something (instead of the hit).
        Audio::SoundEvent hitSound;
        Audio::SoundEvent killSound;

        // How many shots so far hurt something and how many killed something. They only grow: whoever shows hit markers
        // (the crosshair) remembers the numbers it has seen and starts a marker when one grows.
        int hitCount = 0;
        int killCount = 0;

        // How many shots so far, for the pulse of the crosshair (like hitCount).
        int shotCount = 0;

        // Seconds until the next shot is possible; 0 or less: ready.
        float cooldown = 0.0f;

        // A press of Fire in a frame without a tick, kept for the next tick (like the jump, see
        // PlayerController::CollectFrameInput).
        bool isFireRequested = false;

        // The last shot, for the debug lines: where it started and where each pellet ended. secondsSinceLastShot grows
        // every tick, so the lines disappear a while after the shot.
        glm::vec3 lastShotStart{0.0f};
        std::vector<PelletTrace> lastShotPellets;
        float secondsSinceLastShot = 1000.0f;

        // The debug lines of the last shot are drawn (switched in the Weapon window of the debug overlay).
        bool areShotLinesVisible = false;

        // Chooses the directions of the pellets.
        Core::Random random;
    };

    // The directions of the pellets of one shot: count unit vectors spread evenly over the cone of half angle spreadAngle
    // around forward (a unit vector), chosen at random. up is any direction not parallel to forward, to tell which way
    // is "up" in the cone (the up of the view).
    [[nodiscard]] std::vector<glm::vec3> GeneratePelletDirections(const glm::vec3& forward, const glm::vec3& up,
                                                                  float spreadAngle, int count, Core::Random& random);
}
