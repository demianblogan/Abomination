#pragma once

#include "Gameplay/GameplayState.h"

#include <entt/entt.hpp>

namespace Abomination::Gameplay
{
    // Component of the player: the view dips a little after a landing and springs back to the height of the eyes, as the
    // legs of the player take the weight. Only the view moves; the body stays where the physics put it.
    struct LandingDip
    {
        // How far the view is pushed down per meter per second of the fall (meters per second of its own motion), and
        // how stiff the spring is that brings it back (see UpdateDampedSpring).
        float kickPerFallSpeed = 0.8f;
        float springStiffness = 170.0f;

        // How far the view is below the eyes now (meters, negative: down) and how fast it moves.
        float offset = 0.0f;
        float velocity = 0.0f;

        // The body in the last frame, to notice a landing and to know how fast it fell.
        bool wasOnGround = true;
        float previousVerticalSpeed = 0.0f;
    };

    // Landings slower than this (meters per second) do not move the view: walking down a stair is a small fall too.
    inline constexpr float MinimumLandingDipSpeed = 4.0f;

    // Moves the landing dip of one frame (deltaTime, seconds) from what the body did: a landing faster than
    // MinimumLandingDipSpeed kicks the view down in proportion to the fall.
    void UpdateLandingDip(LandingDip& dip, bool isOnGround, float verticalSpeed, float deltaTime);

    // Once per frame: the landing dip of the player (see UpdateLandingDip).
    void UpdatePlayerLandingDip(const GameplayState& state, entt::registry& registry, float deltaTime);
}
