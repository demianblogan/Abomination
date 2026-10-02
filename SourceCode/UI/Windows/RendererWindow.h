#pragma once

namespace Abomination::Renderer
{
    struct RenderSettings;
    struct RenderStatistics;
}

namespace Abomination::World
{
    struct LevelMeshStatistics;
}

namespace Abomination::UI
{
    // The Renderer window of the debug overlay (Renderer in the menu bar): how the scene is drawn (solid or wireframe,
    // world axes), how much the last frame drew (draw calls, triangles), the numbers of the loaded level and a button that
    // loads the map again. The button only sets isLevelReloadRequested: the application reloads at the start of the next
    // frame, not in the middle of drawing the overlay.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawRendererWindow(bool* isOpen, Renderer::RenderSettings& settings, const Renderer::RenderStatistics& statistics,
                            const World::LevelMeshStatistics& levelStatistics, bool& isLevelReloadRequested);
}
