#pragma once

#include "Audio/SoundEvent.h"
#include "Core/Math/Random.h"

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
    };

    // One pellet of the last shot, for the debug lines.
    struct PelletTrace
    {
        glm::vec3 end{0.0f};
        bool hasHit = false;
    };

    // Component of the player: the weapon they shoot with. The weapon in their hands as they see it is the ViewModel;
    // this is what it does.
    struct Weapon
    {
        WeaponSettings settings;

        Audio::SoundEvent fireSound;

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
        bool areShotLinesVisible = true;

        // Chooses the directions of the pellets.
        Core::Random random;
    };

    // The directions of the pellets of one shot: count unit vectors spread evenly over the cone of half angle spreadAngle
    // around forward (a unit vector), chosen at random. up is any direction not parallel to forward, to tell which way
    // is "up" in the cone (the up of the view).
    [[nodiscard]] std::vector<glm::vec3> GeneratePelletDirections(const glm::vec3& forward, const glm::vec3& up,
                                                                  float spreadAngle, int count, Core::Random& random);
}
