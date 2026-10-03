#pragma once

namespace Abomination::Gameplay
{
    struct Ammo;
    struct Armor;
    struct Health;
}

namespace Abomination::UI
{
    // The Player window of the debug overlay (Player in the menu bar): the health, armor and ammunition of the player,
    // with buttons to hurt them and to fill them up, to try the HUD before there are enemies and pickups.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawPlayerWindow(bool* isOpen, Gameplay::Health& health, Gameplay::Armor& armor, Gameplay::Ammo& ammo);
}
