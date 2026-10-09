#pragma once

#include "Gameplay/Effects/Effects.h"
#include "Renderer/Sprites/SpriteBatch.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <entt/entt.hpp>

// The fires of the level (see Renderer/Fire.h): flames played from their flipbooks, the sparks and smoke they throw, and
// the flicker of their light. Only for the eyes, updated every frame like the particles.
namespace Abomination::Gameplay
{
    // The part of a flipbook (16 frames of a 4 x 4 sheet, left to right, top to bottom) that is frame number frame:
    // texture coordinates of its bottom left and top right corners ((0, 0) is the bottom left of the sheet).
    struct FlipbookFrame
    {
        glm::vec2 minimum{0.0f};
        glm::vec2 maximum{1.0f};
    };
    [[nodiscard]] FlipbookFrame CalculateFlipbookFrame(int frame);

    // How much a flickering light is brighter (above 0) or dimmer (below 0) at time (seconds): -1..1, three waves of
    // different speeds added, so the light never repeats in an obvious rhythm. speed is that of the slowest wave (radians
    // per second), phase where it starts.
    [[nodiscard]] float CalculateFlicker(float time, float speed, float phase);

    // Advances the time of the effects by deltaTime, sets the intensity and place of every flickering light, and makes
    // the flames throw their sparks and smoke.
    void UpdateFires(entt::registry& registry, Effects& effects, float deltaTime);

    // Adds every flame as a sprite standing upright, turned to the camera around the vertical (a flame does not lie down
    // when looked at from above), in the frame of its flipbook for the time of the effects.
    void AddFlameSprites(const entt::registry& registry, const Effects& effects, const glm::vec3& cameraPosition,
                         Renderer::SpriteBatch& batch);
}
