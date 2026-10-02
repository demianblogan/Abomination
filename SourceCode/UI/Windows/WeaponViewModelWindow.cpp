#include "UI/Windows/WeaponViewModelWindow.h"

#include "Gameplay/Player/LandingDip.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "UI/UIScale.h"
#include "UI/Widgets.h"

#include <imgui.h>

#include <cstddef>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time at the right edge, below the Collisions window.
        constexpr ImVec2 InitialPosition(900.0f, 620.0f);
    }

    void DrawWeaponViewModelWindow(bool* isOpen, Gameplay::WeaponViewModel& weaponViewModel, Gameplay::LandingDip& landingDip)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Weapon View Model", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        ImGui::SeparatorText("Side");
        for (std::size_t index = 0; index < Gameplay::WeaponViewModelSideNames.size(); ++index)
        {
            if (index > 0)
                ImGui::SameLine();

            const auto side = static_cast<Gameplay::WeaponViewModelSide>(index);
            if (ImGui::RadioButton(Gameplay::WeaponViewModelSideNames[index].data(), weaponViewModel.side == side))
                weaponViewModel.side = side;
        }

        ImGui::SeparatorText("Position from the eyes");
        // Not called "Right": ImGui makes the ID of an item from its label, and the radio button above is "Right" already.
        DrawCentimeterSlider("Sideways", weaponViewModel.offset.x, 0.0f, 50.0f,
                             "How far to the side the middle of the weapon is. For the left side it goes to the left,\n"
                             "in the center it is ignored.");
        DrawCentimeterSlider("Up", weaponViewModel.offset.y, -50.0f, 10.0f, "Negative: below the eyes.");
        DrawCentimeterSlider("Forward", weaponViewModel.offset.z, -100.0f, 0.0f,
                             "Negative: in front of the eyes. The middle of the shotgun: it is 1.1 m long, so at -35 cm\n"
                             "its muzzle is about 90 cm in front of the eyes and its stock behind them.");

        // Found from the model (the middle of the end of its barrel); fine-tuned here if the flash is off.
        ImGui::SeparatorText("Muzzle (in the model)");
        ImGui::PushID("Muzzle");
        constexpr const char* MuzzleTooltip = "Where the flash and the smoke appear.";
        DrawCentimeterSlider("Sideways", weaponViewModel.muzzle.x, -20.0f, 20.0f, MuzzleTooltip);
        DrawCentimeterSlider("Up", weaponViewModel.muzzle.y, -20.0f, 20.0f, MuzzleTooltip);
        DrawCentimeterSlider("Forward", weaponViewModel.muzzle.z, -80.0f, 0.0f, MuzzleTooltip);
        ImGui::PopID();

        ImGui::SeparatorText("Lens");
        DrawDegreeSlider("Vertical FOV", weaponViewModel.verticalFOV, 30.0f, 90.0f, "%.0f deg",
                         "The field of view the weapon alone is drawn with. Smaller: the weapon looks bigger and\n"
                         "flatter; larger: smaller and more stretched in depth. The world is not affected.");

        Gameplay::WeaponViewModelMotionSettings& motion = weaponViewModel.motionSettings;
        ImGui::SeparatorText("Idle (standing)");
        DrawCentimeterSlider("Idle amount", motion.idleAmount, 0.0f, 2.0f,
                             "How far the weapon rises and falls with the breath while the player stands. 0 turns it off.");
        DrawSlider("Breath duration", motion.idleBreathDuration, 1.0f, 10.0f, "%.1f s",
                   "How long one breath takes. Shorter: faster, as if out of breath.");

        ImGui::SeparatorText("Bob (walking)");
        DrawCentimeterSlider("Bob amount", motion.bobAmount, 0.0f, 5.0f,
                             "How far the weapon swings to the sides at full speed. 0 turns the bob off.");
        DrawSlider("Stride length", motion.bobStrideLength, 0.5f, 5.0f, "%.2f m",
                   "How far the player walks during one whole swing (two steps). Shorter: faster swings.");

        ImGui::SeparatorText("Sway (turning)");
        DrawCentimeterSlider("Sway amount", motion.swayAmount, 0.0f, 10.0f,
                             "How far the weapon lags per radian (57 degrees) the view turns. 0 turns the sway off.");
        DrawCentimeterSlider("Sway maximum", motion.swayMaximum, 0.0f, 10.0f, "The farthest the weapon ever lags.");
        DrawSlider("Sway return", motion.swayReturnRate, 1.0f, 30.0f, "%.1f /s",
                   "How fast the weapon catches up with the view. Bigger: quicker, stiffer.");

        ImGui::SeparatorText("Inertia (jumping, landing)");
        DrawSlider("Jump kick", motion.jumpKick, 0.0f, 1.0f, "%.2f m/s",
                   "How hard a jump pushes the weapon down. 0 turns it off.");
        DrawSlider("Landing kick", motion.landingKickPerFallSpeed, 0.0f, 0.1f, "%.3f",
                   "How hard a landing pushes the weapon down, per m/s of the fall (a jump lands at 8.4 m/s).");
        DrawSlider("Spring stiffness", motion.springStiffness, 20.0f, 500.0f, "%.0f",
                   "How fast the weapon comes back after a kick. Bigger: quicker and smaller dips.");

        ImGui::SeparatorText("Camera landing dip");
        DrawSlider("Dip kick", landingDip.kickPerFallSpeed, 0.0f, 1.0f, "%.3f",
                   "How far the view dips after a landing, per m/s of the fall. 0 turns it off.");
        DrawSlider("Dip stiffness", landingDip.springStiffness, 20.0f, 500.0f, "%.0f",
                   "How fast the view comes back up. Bigger: quicker and smaller dips.");

        if (ImGui::Button("Reset"))
        {
            // Only the settings go back to the defaults; the model, the shader, the muzzle found in the model and the
            // current motion stay.
            const Gameplay::WeaponViewModel defaults;
            weaponViewModel.offset = defaults.offset;
            weaponViewModel.side = defaults.side;
            weaponViewModel.verticalFOV = defaults.verticalFOV;
            weaponViewModel.motionSettings = defaults.motionSettings;

            const Gameplay::LandingDip dipDefaults;
            landingDip.kickPerFallSpeed = dipDefaults.kickPerFallSpeed;
            landingDip.springStiffness = dipDefaults.springStiffness;
        }

        ImGui::End();
    }
}
