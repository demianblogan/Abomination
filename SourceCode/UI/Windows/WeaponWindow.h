#pragma once

namespace Abomination::Gameplay
{
    struct ViewRecoil;
    struct Weapon;
    struct WeaponViewModel;
}

namespace Abomination::UI
{
    // The Weapon window of the debug overlay (Weapon in the menu bar): everything about the shotgun in one window, a tab per
    // topic, to balance it and make it feel right while playing:
    //   Shot      - pellets, spread, range, rhythm, damage;
    //   Crosshair - the circle, the dot and the hit and kill markers;
    //   Recoil    - how hard a shot jerks the view and the weapon in the hands;
    //   In hands  - where the weapon is held and drawn, and how it breathes, swings, lags and dips.
    // The values are not saved between runs yet (weapon configuration files come in 0.6); "Reset" on a tab brings back the
    // defaults of that tab.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawWeaponWindow(bool* isOpen, Gameplay::Weapon& weapon, Gameplay::ViewRecoil& viewRecoil,
                          Gameplay::WeaponViewModel& weaponViewModel);
}
