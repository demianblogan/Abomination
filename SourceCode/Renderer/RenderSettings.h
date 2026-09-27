#pragma once

namespace Abomination::Renderer
{
    // How the scene is drawn. Changed from the debug overlay (View > Renderer); later also by the options menu.
    struct RenderSettings
    {
        // Draw only the edges of triangles instead of filled surfaces: shows how surfaces are cut into triangles,
        // and reveals holes, extra triangles or faces turned the wrong way.
        bool isWireframeEnabled = false;

        // Draw the axes of the world at its origin as arrows over everything: X red, Y (up) green, Z blue, 1 meter long.
        // Shows where the origin is and which way the axes point.
        bool areWorldAxesVisible = false;
    };
}
