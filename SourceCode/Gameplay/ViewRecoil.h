#pragma once

#include "Gameplay/GameplayState.h"

#include <entt/entt.hpp>

namespace Abomination::Gameplay
{
    // Component of the player: the view jerks up after a shot and springs back. Only the picture moves: the look angles
    // (where the player aims, where the next shot goes) stay, so the recoil is felt but does not throw the aim off, like
    // in Quake and Doom (unlike games where the player must pull the mouse down against the recoil).
    struct ViewRecoil
    {
        // How hard a shot turns the view up (radians per second of its own motion) and how stiff the spring is that
        // brings it back (see UpdateDampedSpring). With the defaults a shot turns the view up by about 3 degrees.
        float kick = 1.5f;
        float springStiffness = 200.0f;

        // How far the view is turned up now (radians) and how fast it turns.
        float pitch = 0.0f;
        float pitchVelocity = 0.0f;
    };

    // A shot: kicks the view up.
    void KickViewRecoil(ViewRecoil& recoil);

    // Once per frame: the view of the player springs back after the recoil of a shot.
    void UpdatePlayerViewRecoil(const GameplayState& state, entt::registry& registry, float deltaTime);
}
