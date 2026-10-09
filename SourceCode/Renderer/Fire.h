#pragma once

#include <glm/vec3.hpp>

// Fire: the flames of torches, braziers and candles, and the light of a fire that never stands still. Only data: the
// level places them (see World/LightSources.h), Gameplay/Effects/Fires.h animates and draws them.
namespace Abomination::Renderer
{
    // Which flipbook a flame plays (Tools/TextureGenerator, RunFire).
    enum class FlameKind
    {
        // Torn tongues that break off: burning wood, coals (Textures/Effects/Fire.png).
        Wild,

        // A calm drop that only sways: a candle, a wick (Textures/Effects/CandleFire.png).
        Calm,
    };

    // Component: a flame burning at the entity, whose Core::Transform is the bottom of the flame. It is drawn as an
    // animated sprite standing upright and turned to the camera, and may throw sparks and smoke.
    struct Flame
    {
        FlameKind kind = FlameKind::Wild;

        // Meters, from the bottom of the flame to its tip.
        float height = 0.25f;

        // Where in its animation and its wavering the flame starts (radians), so that flames side by side do not move
        // together.
        float phase = 0.0f;

        // Sparks and puffs of smoke per second (0: none). The fractions left over carry to the next frame.
        float sparksPerSecond = 0.0f;
        float smokePerSecond = 0.0f;
        float sparksDue = 0.0f;
        float smokeDue = 0.0f;
    };

    // Component: the light of the entity (Renderer::Light) wavers like the light of a fire: brighter and dimmer, and a
    // little to the sides, so the light and later the shadows it casts move as the flame does.
    struct Flicker
    {
        // The light and the place it wavers around.
        float baseIntensity = 5.0f;
        glm::vec3 basePosition{0.0f};

        // How much it wavers: 0.2 is +-20% of the intensity; wander is how far it moves (meters); speed how fast
        // (radians per second of its slowest wave).
        float strength = 0.2f;
        float wander = 0.02f;
        float speed = 8.0f;

        float phase = 0.0f;
    };
}
