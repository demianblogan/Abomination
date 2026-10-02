#pragma once

namespace Abomination::Gameplay
{
    struct Effects;
}

namespace Abomination::UI
{
    // The Effects window of the debug overlay (Effects in the menu bar): how many particles the effects of shots make,
    // how fast, how big and how long they live, the muzzle flash and the marks on walls. The values are not saved between
    // runs yet; "Reset" brings back the defaults, "Clear" removes every particle and mark.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawEffectsWindow(bool* isOpen, Gameplay::Effects& effects);
}
