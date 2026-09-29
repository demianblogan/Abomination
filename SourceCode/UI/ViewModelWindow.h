#pragma once

namespace Abomination::Gameplay
{
    struct ViewModel;
}

namespace Abomination::UI
{
    // The View Model window of the debug overlay (View > Gameplay > View Model): where the weapon in the hands is held
    // and how wide it is drawn, to find values that look right while playing. The values are not saved between runs yet
    // (that comes with the configuration files, 0.6); "Reset" brings back the default values.
    //
    // Draws the window while *isOpen is true; its close button sets *isOpen to false.
    void DrawViewModelWindow(bool* isOpen, Gameplay::ViewModel& viewModel);
}
