#pragma once

#include "Gameplay/GameplayState.h"

#include <entt/entt.hpp>

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

// The weapon system: the player's shots.
namespace Abomination::Gameplay
{
    // Once per frame: keeps a press of Fire for the next tick, so a short click in a frame without a tick is not lost.
    // canShoot: the player is controlled and the mouse is captured (a click on the debug overlay is not a shot).
    void CollectWeaponInput(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                            bool canShoot);

    // Once per tick: counts down the time to the next shot and fires when Fire is held or was pressed since the last
    // tick and the weapon is ready. A shot sends every pellet from the eyes of the player along its direction in the
    // cone (see GeneratePelletDirections) through the level, and plays the sound of the shot. Returns whether it fired.
    //
    // Shots start at the eyes, not at the muzzle of the weapon in the hands, like in nearly every shooter: the pellets
    // go where the middle of the screen points, even when the barrel is at the edge of the screen or close to a wall.
    bool UpdateWeapon(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions, bool canShoot,
                      std::span<const World::CollisionBrush> brushes, Audio::AudioEngine& audio, float tickDuration);

    // Draws the pellets of the last shot for a moment after it: a line from the start to the end of every pellet and a
    // small box where it hit.
    void AddWeaponDebugLines(const GameplayState& state, const entt::registry& registry, Renderer::DebugLines& debugLines);
}
