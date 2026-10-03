#pragma once

#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <string>
#include <vector>

namespace Abomination::Gameplay
{
    // The cycle of a shot of a pump-action shotgun, one phase after another:
    //   1. the recoil plays out;
    //   2. the weapon is brought to the chest: turned so the barrel swings across towards the middle of
    //      the body and up a little (around a point along the weapon, see turnPivot) and tilted on its side;
    //   3. the pump is worked while the weapon is held there: back, a short stop, forward;
    //   4. the weapon goes back to where it was held.
    // Only then the next shot is possible. The times are in seconds; their sum is the time between shots (see
    // CalculateShotCycleDuration).
    struct PumpActionSettings
    {
        float recoilDuration = 0.35f;

        // Brought to the chest, and back to the aim after the pump.
        float raiseDuration = 0.12f;
        float lowerDuration = 0.145f;

        // The pump: pulled back fast, held for a moment at the back, pushed forward.
        float backDuration = 0.12f;
        float holdDuration = 0.08f;
        float forwardDuration = 0.185f;

        // How far the pump goes back along the barrel (meters).
        float travel = 0.099f;

        // The pose at the chest: the turn of the barrel towards the middle of the body and up (radians),
        // and the tilt on its side around the barrel (radians). The turn and the tilt go to the left (counter-clockwise as
        // the player sees it) for a weapon held on the right or in the middle, to the right for one held on the left.
        float turnAngle = glm::radians(30.0f);
        float liftAngle = glm::radians(6.0f);
        float tiltAngle = glm::radians(50.0f);

        // Where along the weapon the turn and the lift go around: 0 at the end of the stock, 0.5 at the middle of the
        // model, 1 at the muzzle.
        float turnPivot = 0.6f;
    };

    // When the first clack of the pump (hitting the back) is heard in its recording (Sounds/Weapons/Shotgun/Pump1.ogg,
    // seconds from its start). The sound starts early enough that it comes when the pump reaches the back; the gap to the
    // second clack (0.265 s) is the hold and the forward movement of the default settings.
    inline constexpr float PumpSoundBackClackTime = 0.265f;

    // The pump action of the weapon in the hands: the parts of the model that move with the pump (the pump itself and the
    // slide behind it), moved by code after every shot (the model has no animation of its own), and the pose now.
    struct PumpAction
    {
        // The names of the parts of the model that move with the pump ("Pump_low_Shotgun_0").
        std::vector<std::string> partNames;

        // The length of the model along its barrel (meters): the stock ends half of it behind the middle (+Z), the muzzle
        // half of it in front (-Z). The turn goes around a point on this line (see PumpActionSettings::turnPivot).
        float modelLength = 0.0f;
        float turnPivot = 0.0f;

        // Seconds since the last shot; large before the first one, so everything is at rest.
        float secondsSinceShot = 1000.0f;

        // Now: how far the pump is back (meters), and the pose at the chest (radians: the turn of the barrel to the left
        // and up, the counter-clockwise tilt around the barrel). Set every frame by UpdateWeaponViewModel.
        float travel = 0.0f;
        float turn = 0.0f;
        float lift = 0.0f;
        float tilt = 0.0f;

        // The pump has just reached the back: the spent shell is to be thrown out (see UpdateShells, which clears it).
        bool isShellEjectRequested = false;
    };

    // How far through its movement the pump is secondsSinceShot after a shot: 0 at rest, 1 at the back. It goes back with
    // a quick start and a soft stop (ease-out), stays, and comes forward smoothly (smoothstep), like a hand working it.
    [[nodiscard]] float CalculatePumpProgress(const PumpActionSettings& settings, float secondsSinceShot);

    // How far the weapon is brought to the chest secondsSinceShot after a shot: 0 at the aim, 1 at the chest. It is
    // raised smoothly, stays there while the pump is worked, and is lowered smoothly.
    [[nodiscard]] float CalculateChestPose(const PumpActionSettings& settings, float secondsSinceShot);

    // When the pump starts moving back, seconds after the shot (after the recoil and the raise).
    [[nodiscard]] float CalculatePumpStartTime(const PumpActionSettings& settings);

    // The whole cycle of a shot. The next shot is possible after it.
    [[nodiscard]] float CalculateShotCycleDuration(const PumpActionSettings& settings);
}
