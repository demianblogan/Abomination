#pragma once

namespace Abomination::Gameplay
{
    struct ViewModelMotionSettings;
    struct ViewRecoil;
    struct Weapon;
}

namespace Abomination::UI
{
    // The Weapon window of the debug overlay (View > Gameplay > Weapon): how the shotgun shoots (pellets, spread, range,
    // rhythm) and how hard its recoil jerks the view and the weapon in the hands, to balance it while playing. The values
    // are not saved between runs yet (weapon configuration files come in 0.6); "Reset" brings back the defaults.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawWeaponWindow(bool* isOpen, Gameplay::Weapon& weapon, Gameplay::ViewRecoil& viewRecoil,
                          Gameplay::ViewModelMotionSettings& viewModelMotion);
}
