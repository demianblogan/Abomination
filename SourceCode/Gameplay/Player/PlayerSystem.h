#pragma once

#include "Gameplay/GameplayState.h"

#include <entt/entt.hpp>
#include <glm/vec2.hpp>

#include <span>

namespace Abomination::Audio
{
    class AudioEngine;
}

namespace Abomination::Input
{
    class ActionStates;
}

namespace Abomination::Renderer
{
    class DebugLines;
}

namespace Abomination::World
{
    struct CollisionBrush;
}

// The player system: what happens to the player entity every frame and every tick.
namespace Abomination::Gameplay
{
    // Once per frame, while the player is controlled: keeps the presses of this frame for the next tick (see
    // PlayerController::CollectFrameInput) and turns the view by mouseMovement (pixels). Whoever owns the window passes
    // no movement while the mouse must not turn the view (the cursor is free for the debug overlay).
    void UpdatePlayerLook(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                          glm::vec2 mouseMovement);

    // Once per tick: the player moves through the level (see Physics::UpdateCharacter) and the eyes follow steps
    // smoothly. The world goes on while the free-fly camera looks, so the player keeps moving (falling) then, but takes
    // commands from the keys only while controlled.
    void UpdatePlayer(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                      std::span<const World::CollisionBrush> brushes, float tickDuration);

    // A landing slower than this (meters per second, downwards) is silent: walking down a stair is a small fall too.
    // The fall after a jump from the ground lands at about the jump speed, 8.4 m/s.
    inline constexpr float MinimumLandingSoundSpeed = 4.0f;

    // A landing this fast or faster plays at full volume: a fall of about 4 m with the default gravity of 25 m/s²
    // (speed² / (2 * gravity) = 14² / 50). An ordinary jump lands at about two thirds of the volume.
    inline constexpr float FullLandingSoundSpeed = 14.0f;

    // The volume of the landing sound for the speed of the fall (meters per second, downwards, positive): 0 below
    // MinimumLandingSoundSpeed, then from a third of the volume up to full at FullLandingSoundSpeed.
    [[nodiscard]] float CalculateLandingVolume(float fallSpeed);

    // Once per tick, after UpdatePlayer: plays the jump when the player leaves the ground upwards, and the landing when
    // they come down on it (see CalculateLandingVolume). The sounds of the player's own body play "in the head" (2D);
    // while the free-fly camera looks, they play in 3D where the player is, so they can be heard from far away.
    void UpdatePlayerSounds(GameplayState& state, const entt::registry& registry, Audio::AudioEngine& audio);

    // Seen from the free-fly camera, the player is drawn as a box (it has no model yet), so it is clear where they stand.
    // Adds nothing while the player is controlled.
    void AddPlayerDebugBox(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                           Renderer::DebugLines& debugLines);

    // Draws the box of every character other than the player (the target dummies, later enemies): the box they collide
    // with and pellets hit. Shown together with the brush bounds (Draw collider bounds in the Collisions window).
    void AddCharacterDebugBoxes(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                                Renderer::DebugLines& debugLines);
}
