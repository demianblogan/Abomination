#pragma once

#include "Core/Scene/Transform.h"

#include <entt/entt.hpp>

namespace Abomination::Gameplay
{
    struct GameplayState;

    // The light of a shot: for a moment the muzzle lights the room, the dogs and the weapon itself, fading out fast (its
    // intensity, range, color and duration are effect settings, see EffectSettings). It is an entity with a
    // Renderer::Light like the lamps of the level, created at the first shot; between shots it has no Light, so it costs
    // nothing.
    struct MuzzleLight
    {
        entt::entity entity = entt::null;

        // How much longer it shines (seconds; 0 or less: dark).
        float timeLeft = 0.0f;
    };

    // A shot: the light flashes at full intensity.
    void FlashMuzzleLight(GameplayState& state);

    // Places the light at the muzzle of the weapon in the hands, seen from eyes (the eyes of the player between the last
    // two ticks), and fades it: its intensity falls with the square of the time left, quickly at first, like a flash.
    void UpdateMuzzleLight(GameplayState& state, entt::registry& registry, const Core::Transform& eyes, float deltaTime);
}
