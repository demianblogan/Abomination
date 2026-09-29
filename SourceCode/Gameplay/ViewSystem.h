#pragma once

#include "Core/Scene/Transform.h"
#include "Gameplay/GameplayState.h"

#include <entt/entt.hpp>

namespace Abomination::Renderer
{
    struct CameraLens;
}

// The view system: which entity the scene is drawn through, and switching between the player and the free-fly camera.
namespace Abomination::Gameplay
{
    // Switches between the player and the free-fly camera. The free-fly camera starts at the eyes of the player, looking
    // the same way, so the view does not jump.
    void ToggleFreeFlyCamera(GameplayState& state, entt::registry& registry);

    // Where the scene is drawn from this frame: the eyes of the player or the free-fly camera, between their last two
    // ticks (see Core::InterpolateTransform).
    [[nodiscard]] Core::Transform CalculateViewTransform(const GameplayState& state, const entt::registry& registry,
                                                         float interpolationFactor);

    // The camera lens of the entity the scene is drawn through.
    [[nodiscard]] const Renderer::CameraLens& GetViewLens(const GameplayState& state, const entt::registry& registry);
}
