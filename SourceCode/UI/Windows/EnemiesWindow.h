#pragma once

#include <entt/entt.hpp>

namespace Abomination::Gameplay
{
    struct GameplayState;
}

namespace Abomination::UI
{
    // The Enemies window of the debug overlay (Enemies in the menu bar), a tab per kind of monster. Dog: every dog of the
    // level with its state and health; how all dogs see, smell, hear, walk, run, turn, patrol, leap and bite; their
    // senses drawn in the level; and a switch that freezes every monster. The values are not saved between runs yet;
    // "Reset" brings back the defaults.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawEnemiesWindow(bool* isOpen, Gameplay::GameplayState& gameplay, entt::registry& registry);
}
