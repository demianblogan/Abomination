#pragma once

#include <entt/entt.hpp>

namespace Abomination::Audio
{
    class AudioEngine;
}

namespace Abomination::Gameplay
{
    struct GameplayState;
}

namespace Abomination::UI
{
    // The Player window of the debug overlay (Player in the menu bar): the health, armor and ammunition of the player,
    // with buttons to hurt them (a blow from a random side) and to heal and fill them up, to try the HUD and the reaction
    // to damage before there are enemies and pickups; and the values of that reaction (the punch of the view, the muffle,
    // the heartbeat) to tune it while playing.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawPlayerWindow(bool* isOpen, Gameplay::GameplayState& gameplay, entt::registry& registry,
                          Audio::AudioEngine& audio);
}
