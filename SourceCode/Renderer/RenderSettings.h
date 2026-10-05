#pragma once

namespace Abomination::Renderer
{
    // How the HDR scene (linear values from 0 to far above 1) is fitted into the 0..1 the screen can show, in the Present
    // pass (see Present.frag). The numbers are the ones the shader compares uniToneMapping with.
    enum class ToneMapping
    {
        // Everything above 1 is cut off: a flame and a white wall look the same. For comparison.
        None = 0,

        // The ACES filmic curve (as approximated by Krzysztof Narkowicz): dark values almost unchanged, bright ones
        // pressed together more and more, never cut off. Used by films and many games.
        ACES = 1,
    };

    // How the scene is drawn. Changed from the debug overlay (Renderer in the menu bar); later also by the options menu.
    struct RenderSettings
    {
        // Draw only the edges of triangles instead of filled surfaces: shows how surfaces are cut into triangles,
        // and reveals holes, extra triangles or faces turned the wrong way.
        bool isWireframeEnabled = false;

        // Draw the axes of the world at its origin as arrows over everything: X red, Y (up) green, Z blue, 1 meter long.
        // Shows where the origin is and which way the axes point.
        bool areWorldAxesVisible = false;

        // The brightness of the scene before tone mapping, in stops like a camera: every +1 doubles it, every -1 halves
        // it, 0 leaves it as it is. A dark crypt will want more, a sunny street less.
        float exposureStops = 0.0f;

        ToneMapping toneMapping = ToneMapping::ACES;
    };
}
