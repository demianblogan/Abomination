#pragma once

namespace Abomination::Renderer
{
    struct RenderAssets;
}

namespace Abomination::UI
{
    // The Assets window of the debug overlay (View > Engine > Assets): every loaded texture, mesh and shader program, with
    // its size, video memory, lifetime group and whether a fallback replaced a missing or broken file.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawAssetsWindow(bool* isOpen, const Renderer::RenderAssets& assets);
}
