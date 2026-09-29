#pragma once

namespace Abomination::Renderer
{
    // Routes messages of the graphics driver (errors, warnings, performance hints) to the log, category Renderer.
    // Works only with a debug context (requested in Debug builds); otherwise it logs that and does nothing.
    // Requires loaded OpenGL functions.
    void EnableDebugOutput();
}
