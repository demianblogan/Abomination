#pragma once

// Smooth motions that behave the same at any frame rate: the springs of the weapon in the hands, the recoil, the landing
// dip of the view and the pulse of the crosshair, and values that glide towards a target (the sway of the weapon).
namespace Abomination::Core
{
    // Moves a damped spring by deltaTime (seconds): offset is pulled back to 0 and settles after one slight swing past it;
    // a kick is a change of velocity. stiffness is per second squared: stiffer returns faster. Stable at any frame rate:
    // a long frame is moved in small steps.
    //
    // With stiffness k the spring swings about sqrt(k) radians per second; a kick of velocity v moves it at most about
    // 0.46 * v / sqrt(k) away (the 0.46 comes from the damping, see Spring.cpp). Example: k = 150, v = 1: 3.8 cm.
    void UpdateDampedSpring(float& offset, float& velocity, float stiffness, float deltaTime);

    // The part of a gap that is closed during deltaTime when rate of it is closed per second, the same way at any frame
    // rate: after two frames of 0.01 s exactly as much is closed as after one frame of 0.02 s. (Closing rate * deltaTime
    // of it each frame, the simple way, would close less at a low frame rate.) Use: value += (target - value) * factor.
    [[nodiscard]] float CalculateApproachFactor(float rate, float deltaTime);
}
