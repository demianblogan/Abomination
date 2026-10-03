#pragma once

#include <entt/entt.hpp>

namespace Abomination::Renderer
{
    class ModelStore;
}

namespace Abomination::UI
{
    // The Animation window of the debug overlay (Animation in the menu bar): every animated model of the level
    // (see Gameplay::Animator), its segments to play with a cross-fade, its speed, a time line to scrub through the
    // segment, the start and end of the segment in its clip (to find where the animations of one long clip are), and its
    // skeleton drawn as lines. "Log segments" writes the segments to the console as code, ready to be put into
    // Gameplay::FindModelSegments.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawAnimationWindow(bool* isOpen, entt::registry& registry, const Renderer::ModelStore& models);
}
