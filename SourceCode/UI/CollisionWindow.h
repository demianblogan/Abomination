#pragma once

#include <cstddef>

namespace Abomination::World
{
    struct CameraTrace;
    struct CollisionDebugSettings;
}

namespace Abomination::UI
{
    // The Collision window of the debug overlay (View > Physics > Collision): the collision tools (see
    // World::CollisionDebugSettings) and the result of the trace from the camera in the last frame.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawCollisionWindow(bool* isOpen, World::CollisionDebugSettings& settings, const World::CameraTrace& cameraTrace,
                             std::size_t collisionBrushCount);
}
