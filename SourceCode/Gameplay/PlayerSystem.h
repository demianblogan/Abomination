#pragma once

#include "Gameplay/GameplayState.h"

#include <entt/entt.hpp>
#include <glm/vec2.hpp>

#include <span>

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

    // Seen from the free-fly camera, the player is drawn as a box (it has no model yet), so it is clear where they stand.
    // Adds nothing while the player is controlled.
    void AddPlayerDebugBox(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                           Renderer::DebugLines& debugLines);
}
