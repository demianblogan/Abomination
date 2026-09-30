#pragma once

#include "Gameplay/MouseLook.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// The movement of the weapon in the hands, which makes it feel held instead of glued to the screen. The motions add up:
//   idle    - it rises and falls slowly with the breath while the player stands;
//   bob     - it swings in a figure eight while walking, faster and wider the faster the player runs;
//   sway    - it lags behind the view when the mouse turns it, then catches up;
//   inertia - it dips when the body leaves the ground in a jump and when it lands, and springs back.
// All of it is only for the eyes: the weapon shoots from the middle of the screen wherever it swings.
namespace Abomination::Gameplay
{
    // Values a designer tunes (in the View Model window of the debug overlay).
    struct ViewModelMotionSettings
    {
        // Bob: how far the weapon swings to each side and up and down at full speed (meters), and how far the player
        // walks during one whole figure eight (two steps, meters).
        float bobAmount = 0.016f;
        float bobStrideLength = 5.0f;

        // Idle: while the player stands, the weapon rises and falls slowly with the breath and drifts a little to the
        // sides, like in real hands. How far (meters) and how long one breath takes (seconds). It fades out as the bob
        // fades in.
        float idleAmount = 0.003f;
        float idleBreathDuration = 4.0f;

        // Sway: how far the weapon lags per radian the view turns (meters), at most how far (meters), and how fast it
        // catches up (per second: 10 closes 63% of the gap in a tenth of a second).
        float swayAmount = 0.045f;
        float swayMaximum = 0.05f;
        float swayReturnRate = 10.0f;

        // Inertia: how much the weapon is pushed down by a jump (meters per second of its own motion), how much by a
        // landing per meter per second of the fall, and how stiff the spring is that brings it back (per second squared:
        // stiffer returns faster). The spring is damped so it settles after one small swing.
        float jumpKick = 0.38f;
        float landingKickPerFallSpeed = 0.085f;
        float springStiffness = 150.0f;

        // Recoil of a shot: how hard it pushes the weapon back towards the eyes (meters per second) and turns its muzzle
        // up (radians per second), and how stiff the springs are that bring it back. With the defaults a shot moves it
        // about 20 cm back (a hard shotgun kick) and 4.5 degrees up.
        float recoilKickBack = 5.0f;
        float recoilKickUp = 2.0f;
        float recoilStiffness = 140.0f;
    };

    // What the motion needs to know about the player each frame.
    struct ViewModelMotionInput
    {
        // Horizontal speed now and the fastest the player walks on their own (meters per second).
        float horizontalSpeed = 0.0f;
        float maxSpeed = 7.0f;

        bool isOnGround = true;

        // Meters per second, positive upwards.
        float verticalSpeed = 0.0f;

        // Where the player looks now (see MouseLook.h).
        LookAngles look;
    };

    // The state of the motion between frames.
    struct ViewModelMotion
    {
        // How far through the figure eight the bob is (radians), and how strongly it swings (0 standing, 1 at full
        // speed): it fades in and out instead of starting and stopping at once.
        float bobPhase = 0.0f;
        float bobWeight = 0.0f;

        // Seconds of breathing (see idleAmount); goes around every two breaths.
        float idleTime = 0.0f;

        // How far the weapon lags behind the view (meters, +X to the right, +Y up).
        glm::vec2 sway{0.0f};

        // How far the weapon is pushed down by inertia (meters, negative: down) and how fast it moves (meters per second).
        float inertiaOffset = 0.0f;
        float inertiaVelocity = 0.0f;

        // The recoil: how far the weapon is pushed back (meters, positive: towards the eyes) and its muzzle turned up
        // (radians), and how fast each of them moves.
        float recoilBack = 0.0f;
        float recoilBackVelocity = 0.0f;
        float recoilPitch = 0.0f;
        float recoilPitchVelocity = 0.0f;

        // The input of the last frame, to see what changed.
        bool wasOnGround = true;
        float previousVerticalSpeed = 0.0f;
        LookAngles previousLook;
        bool hasPreviousLook = false;
    };

    // Moves the motion forward by one frame (deltaTime, seconds). Called every frame, not in ticks: the weapon follows
    // the view, which moves every frame. Frame rate independent: the same movement gives the same motion at any FPS.
    void UpdateViewModelMotion(ViewModelMotion& motion, const ViewModelMotionSettings& settings,
                               const ViewModelMotionInput& input, float deltaTime);

    // Moves a damped spring by deltaTime (seconds): offset is pulled back to 0 and settles after one slight swing past
    // it; a kick is a change of velocity. stiffness is per second squared: stiffer returns faster. Stable at any frame
    // rate: a long frame is moved in small steps. Shared by the inertia of the weapon and the landing dip of the view.
    void UpdateDampedSpring(float& offset, float& velocity, float stiffness, float deltaTime);

    // A shot: kicks the weapon back and its muzzle up (see recoilKickBack); the springs bring it back.
    void KickViewModelRecoil(ViewModelMotion& motion, const ViewModelMotionSettings& settings);

    // How far the motion moves the weapon now, relative to where it is held (meters, eye space: +X right, +Y up, +Z
    // towards the eyes).
    [[nodiscard]] glm::vec3 CalculateViewModelMotionOffset(const ViewModelMotion& motion,
                                                           const ViewModelMotionSettings& settings);
}
