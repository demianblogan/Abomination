#pragma once

#include "Gameplay/GameplayState.h"

#include <entt/entt.hpp>

#include <span>

namespace Abomination::Input
{
    class ActionStates;
    class Mouse;
}

namespace Abomination::World
{
    struct CollisionBrush;
}

// The free-fly camera system: what happens to the free-fly camera every frame and every tick. The camera does nothing
// while the player is controlled.
namespace Abomination::Gameplay
{
    // Once per frame: turns the camera with the mouse while LookAroundMode is active (see
    // FreeFlyCameraController::UpdateRotation).
    void UpdateFreeFlyCameraLook(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                                 const Input::Mouse& mouse);

    // Once per tick: flies the camera. With doesCollide it slides along walls instead of flying through them.
    void UpdateFreeFlyCamera(GameplayState& state, entt::registry& registry, const Input::ActionStates& actions,
                             std::span<const World::CollisionBrush> brushes, bool doesCollide, float tickDuration);
}
