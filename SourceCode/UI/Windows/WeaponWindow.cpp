#include "UI/Windows/WeaponWindow.h"

#include "Gameplay/Weapons/ViewRecoil.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "UI/UIScale.h"
#include "UI/Widgets.h"

#include <imgui.h>

#include <cmath>
#include <cstddef>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time at the right side of the screen.
        constexpr ImVec2 InitialPosition(900.0f, 300.0f);

        // The sliders of one kind of hit marker. PushID keeps the equal labels of the hit and the kill marker apart for
        // ImGui, which makes the ID of an item from its label.
        void DrawHitMarkerSettings(const char* title, Gameplay::HitMarkerSettings& marker)
        {
            ImGui::SeparatorText(title);
            ImGui::PushID(title);

            ImGui::ColorEdit4("Color", &marker.color.r, ImGuiColorEditFlags_NoInputs);
            DrawSlider("Line length", marker.lineLength, 1.0f, 30.0f, "%.1f px");
            DrawSlider("Gap from circle", marker.startGap, -20.0f, 30.0f, "%.1f px",
                       "Where the lines start, from the circle. Negative: inside it.");
            DrawSlider("Travel", marker.travelDistance, -30.0f, 30.0f, "%.1f px",
                       "How far the lines move while they fade. Positive: outwards, negative: inwards.");
            DrawSlider("Duration", marker.duration, 0.05f, 1.0f, "%.2f s");
            DrawSlider("Thickness", marker.thickness, 0.5f, 6.0f, "%.1f px");

            ImGui::PopID();
        }

        void DrawShotTab(Gameplay::Weapon& weapon)
        {
            Gameplay::WeaponSettings& settings = weapon.settings;

            // The kind of ammunition the weapon uses; the HUD shows its icon and count. Until there are more weapons
            // (0.7), switching it is the way to see every ammunition icon in the HUD.
            int ammoType = static_cast<int>(settings.ammoType);
            for (std::size_t index = 0; index < Gameplay::AmmoTypeNames.size(); ++index)
            {
                if (index > 0)
                    ImGui::SameLine();
                ImGui::RadioButton(Gameplay::AmmoTypeNames[index].data(), &ammoType, static_cast<int>(index));
            }
            settings.ammoType = static_cast<Gameplay::AmmoType>(ammoType);
            ImGui::SetItemTooltip("The ammunition the weapon uses (to try the icons of the HUD).");

            DrawIntSlider("Pellets", settings.pelletCount, 1, 20, "How many pellets one shot sends.");
            DrawDegreeSlider("Spread", settings.spreadAngle, 0.0f, 15.0f, "%.1f deg",
                             "How far a pellet may fly off the middle of the screen (half the angle of the cone).\n"
                             "Smaller: a tighter group that hits farther away.");

            // How wide the pellets spread at 10 m, to picture the spread angle.
            const float widthAtTenMeters = 2.0f * 10.0f * std::tan(settings.spreadAngle);
            ImGui::Text("Group at 10 m:  %.2f m wide", widthAtTenMeters);

            DrawSlider("Range", settings.range, 5.0f, 200.0f, "%.0f m", "Pellets fly this far and hit nothing beyond it.");
            DrawSlider("Time between shots", settings.timeBetweenShots, 0.1f, 2.0f, "%.2f s",
                       "The rhythm of the weapon: holding Fire shoots again after this time.");
            DrawSlider("Damage per pellet", settings.damagePerPellet, 0.0f, 50.0f, "%.1f",
                       "Health one pellet takes. A target dummy has 100.");
            DrawSlider("Knockback per pellet", settings.knockbackPerPellet, 0.0f, 5.0f, "%.2f m/s",
                       "How hard one pellet pushes what it hits away from the shooter.");

            ImGui::Checkbox("Show pellet lines", &weapon.areShotLinesVisible);
            ImGui::SetItemTooltip("Lines of the pellets of the last shot for 2 seconds: yellow where they hit a character,\n"
                                  "red with a box where they hit a wall, gray where they flew into nothing. Best seen from\n"
                                  "the free-fly camera (F2).");

            ImGui::Separator();
            if (ImGui::Button("Reset"))
                settings = Gameplay::WeaponSettings{};
        }

        void DrawCrosshairTab(Gameplay::CrosshairSettings& crosshair)
        {
            ImGui::ColorEdit4("Crosshair color", &crosshair.color.r, ImGuiColorEditFlags_NoInputs);
            DrawSlider("Circle thickness", crosshair.circleThickness, 0.5f, 5.0f, "%.1f px",
                       "The circle is as wide as the spread: every pellet lands inside it.");
            DrawSlider("Dot radius", crosshair.dotRadius, 0.0f, 5.0f, "%.1f px");
            DrawSlider("Pulse kick", crosshair.pulseKick, 0.0f, 20.0f, "%.1f",
                       "How much a shot widens the circle for a moment. 0 turns the pulse off.");
            DrawSlider("Pulse stiffness", crosshair.pulseStiffness, 20.0f, 800.0f, "%.0f",
                       "How fast the circle comes back. Bigger: quicker and smaller.");

            DrawHitMarkerSettings("Hit marker", crosshair.hitMarker);
            DrawHitMarkerSettings("Kill marker", crosshair.killMarker);

            ImGui::Separator();
            if (ImGui::Button("Reset"))
                crosshair = Gameplay::CrosshairSettings{};
        }

        void DrawRecoilTab(Gameplay::ViewRecoil& viewRecoil, Gameplay::WeaponViewModelMotionSettings& motion)
        {
            ImGui::SeparatorText("View");
            DrawSlider("View kick", viewRecoil.kick, 0.0f, 2.0f, "%.2f",
                       "How hard a shot jerks the view up. Only the picture moves: the aim stays.");
            DrawSlider("View stiffness", viewRecoil.springStiffness, 20.0f, 500.0f, "%.0f",
                       "How fast the view comes back down. Bigger: quicker and smaller kicks.");

            ImGui::SeparatorText("Weapon in the hands");
            DrawSlider("Kick back", motion.recoilKickBack, 0.0f, 5.0f, "%.2f m/s",
                       "How hard a shot pushes the weapon back towards the eyes.");
            DrawSlider("Kick up", motion.recoilKickUp, 0.0f, 5.0f, "%.2f rad/s", "How hard a shot turns the muzzle up.");
            DrawSlider("Weapon stiffness", motion.recoilStiffness, 20.0f, 500.0f, "%.0f",
                       "How fast the weapon comes back. Bigger: quicker and smaller kicks.");

            ImGui::Separator();
            if (ImGui::Button("Reset"))
            {
                const Gameplay::ViewRecoil recoilDefaults;
                viewRecoil.kick = recoilDefaults.kick;
                viewRecoil.springStiffness = recoilDefaults.springStiffness;

                const Gameplay::WeaponViewModelMotionSettings motionDefaults;
                motion.recoilKickBack = motionDefaults.recoilKickBack;
                motion.recoilKickUp = motionDefaults.recoilKickUp;
                motion.recoilStiffness = motionDefaults.recoilStiffness;
            }
        }

        void DrawInHandsTab(Gameplay::WeaponViewModel& weaponViewModel)
        {
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
            // Not called "Right": ImGui makes the ID of an item from its label, and a radio button above is "Right" already.
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
                                 "How far the weapon rises and falls with the breath while the player stands.\n"
                                 "0 turns it off.");
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

            ImGui::Separator();
            if (ImGui::Button("Reset"))
            {
                // Only the settings of this tab go back to the defaults; the model, the shader, the muzzle found in the
                // model, the recoil (Recoil tab) and the current motion stay.
                const Gameplay::WeaponViewModel defaults;
                weaponViewModel.offset = defaults.offset;
                weaponViewModel.side = defaults.side;
                weaponViewModel.verticalFOV = defaults.verticalFOV;

                const Gameplay::WeaponViewModelMotionSettings recoil = motion;
                motion = defaults.motionSettings;
                motion.recoilKickBack = recoil.recoilKickBack;
                motion.recoilKickUp = recoil.recoilKickUp;
                motion.recoilStiffness = recoil.recoilStiffness;
            }
        }
    }

    void DrawWeaponWindow(bool* isOpen, Gameplay::Weapon& weapon, Gameplay::ViewRecoil& viewRecoil,
                          Gameplay::WeaponViewModel& weaponViewModel)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Weapon", isOpen))
        {
            ImGui::End();
            return;
        }

        // BeginTabBar() starts a row of tabs; BeginTabItem() returns true for the selected one, whose contents follow.
        if (ImGui::BeginTabBar("WeaponTabs"))
        {
            if (ImGui::BeginTabItem("Shot"))
            {
                DrawShotTab(weapon);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Crosshair"))
            {
                DrawCrosshairTab(weapon.crosshair);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Recoil"))
            {
                DrawRecoilTab(viewRecoil, weaponViewModel.motionSettings);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("In hands"))
            {
                DrawInHandsTab(weaponViewModel);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}
