#include "UI/Windows/WeaponWindow.h"

#include "Gameplay/Weapons/Shells.h"
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
            DrawSlider("Damage per pellet", settings.damagePerPellet, 0.0f, 50.0f, "%.1f",
                       "Health one pellet takes. A dog has 90.");
            DrawSlider("Knockback per pellet", settings.knockbackPerPellet, 0.0f, 5.0f, "%.2f m/s",
                       "How hard one pellet pushes what it hits away from the shooter.");

            // The cycle of a shot sets the rhythm: holding Fire shoots again when the pump is forward again.
            Gameplay::PumpActionSettings& pump = settings.pumpAction;
            ImGui::SeparatorText("Shot cycle");
            ImGui::Text("Time between shots: %.2f s", Gameplay::CalculateShotCycleDuration(pump));
            DrawSlider("Recoil time", pump.recoilDuration, 0.0f, 1.0f, "%.2f s",
                       "From the shot until the weapon is brought to the chest: the recoil plays out first.");
            DrawSlider("To the chest", pump.raiseDuration, 0.02f, 0.5f, "%.2f s",
                       "How long the weapon takes to turn and tilt to the chest.");
            DrawSlider("Pump back", pump.backDuration, 0.02f, 0.5f, "%.2f s", "How long the pump takes to go back.");
            DrawSlider("Pump hold", pump.holdDuration, 0.0f, 0.3f, "%.2f s", "How long it stays at the back.");
            DrawSlider("Pump forward", pump.forwardDuration, 0.02f, 0.5f, "%.2f s", "How long it takes to come forward.");
            DrawSlider("Back to aim", pump.lowerDuration, 0.02f, 0.5f, "%.2f s",
                       "How long the weapon takes to come back from the chest.");
            DrawCentimeterSlider("Pump travel", pump.travel, 0.0f, 15.0f, "How far the pump goes back along the barrel.");
            DrawDegreeSlider("Turn", pump.turnAngle, 0.0f, 60.0f, "%.0f deg",
                             "How far the barrel swings across towards the body, around the stock.");
            DrawDegreeSlider("Lift", pump.liftAngle, 0.0f, 20.0f, "%.1f deg",
                             "How far the barrel turns up, around the stock.");
            DrawSlider("Turn point", pump.turnPivot, 0.0f, 1.0f, "%.2f",
                       "Where the turn and the lift go around: 0 the end of the stock, 0.5 the middle, 1 the muzzle.");
            DrawDegreeSlider("Tilt", pump.tiltAngle, 0.0f, 90.0f, "%.0f deg",
                             "How far the weapon tilts on its side towards the body, around its barrel.");

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

            // The hands holding the weapon, posed in Blender (see Gameplay::WeaponHands).
            ImGui::SeparatorText("Hands");
            ImGui::Checkbox("Show hands", &weaponViewModel.hands.isVisible);
            ImGui::SetItemTooltip("The hands are posed in Blender: Tools/Blender/HandsPoseScene.py and ExportHandsPoses.py.");

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

    namespace
    {
        void DrawShellsTab(Gameplay::Shells& shells, entt::registry& registry)
        {
            Gameplay::ShellSettings& settings = shells.settings;
            ImGui::Text("Shells in the level: %zu", shells.shells.size());

            ImGui::SeparatorText("Throw");
            DrawSlider("To the side", settings.sideSpeed, 0.0f, 6.0f, "%.2f m/s",
                       "How fast the shell flies out to the right side of the weapon.");
            DrawSlider("Up", settings.upSpeed, -2.0f, 4.0f, "%.2f m/s", "How fast it flies up (of the weapon).");
            DrawSlider("Back", settings.backSpeed, -2.0f, 3.0f, "%.2f m/s", "How fast it flies back, towards the stock.");
            DrawSlider("Speed variation", settings.speedVariation, 0.0f, 0.6f, "%.2f",
                       "How much every throw changes the speed (0.2: by up to 20%).");
            DrawDegreeSlider("Spin", settings.spinSpeed, 0.0f, 3000.0f, "%.0f deg/s", "How fast it turns end over end.");
            DrawCentimeterSlider("Window right", settings.windowOffset.x, -5.0f, 5.0f,
                                 "Moves the place it comes out of along the weapon's right.");
            DrawCentimeterSlider("Window up", settings.windowOffset.y, -5.0f, 5.0f,
                                 "Moves the place it comes out of along the weapon's up.");
            DrawCentimeterSlider("Window back", settings.windowOffset.z, -20.0f, 20.0f,
                                 "Moves the place it comes out of along the barrel, towards the stock.");

            ImGui::SeparatorText("Bounces");
            DrawSlider("Bounce", settings.bounce, 0.0f, 1.0f, "%.2f",
                       "The part of the speed into a surface it jumps back with.");
            DrawSlider("Slide", settings.slide, 0.0f, 1.0f, "%.2f",
                       "The part of the speed along a surface it keeps after a bounce.");
            DrawSlider("Rest speed", settings.restSpeed, 0.05f, 2.0f, "%.2f m/s", "Slower than this on a floor it lies down.");
            DrawIntSlider("Maximum count", settings.maximumCount, 1, 100,
                          "How many shells lie in the level; a new one replaces the oldest.");
            DrawSlider("Sound from", settings.soundSpeed, 0.0f, 3.0f, "%.2f m/s",
                       "A bounce is heard when the shell hits a surface faster than this.");
            DrawSlider("Full volume at", settings.fullVolumeSpeed, 0.5f, 8.0f, "%.2f m/s");
            DrawSlider("Sound fade-out", settings.soundFadeOut, 0.0f, 1.0f, "%.2f s",
                       "How long the ringing of the last bounce fades out once the shell lies still.");

            ImGui::SeparatorText("Smoke");
            DrawIntSlider("Window puffs", settings.windowSmokeCount, 0, 10, "Puffs of smoke out of the window.");
            DrawSlider("Window lifetime", settings.windowSmokeLifetime, 0.1f, 2.0f, "%.2f s");
            DrawCentimeterSlider("Window size", settings.windowSmokeHalfSize, 0.5f, 20.0f, "Half the size of a puff.");
            DrawSlider("Trail duration", settings.trailDuration, 0.0f, 2.0f, "%.2f s",
                       "How long the shell trails smoke after the throw.");
            DrawSlider("Trail interval", settings.trailInterval, 0.005f, 0.2f, "%.3f s",
                       "Time between two puffs of the trail.");
            DrawSlider("Trail lifetime", settings.trailLifetime, 0.1f, 2.0f, "%.2f s");
            DrawCentimeterSlider("Trail size", settings.trailHalfSize, 0.2f, 10.0f, "Half the size of a puff of the trail.");

            ImGui::Separator();
            if (ImGui::Button("Reset"))
                settings = Gameplay::ShellSettings{};
            ImGui::SameLine();
            if (ImGui::Button("Remove shells"))
                Gameplay::ClearShells(shells, registry);
        }
    }

    void DrawWeaponWindow(bool* isOpen, Gameplay::Weapon& weapon, Gameplay::ViewRecoil& viewRecoil,
                          Gameplay::WeaponViewModel& weaponViewModel, Gameplay::Shells& shells, entt::registry& registry)
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

            if (ImGui::BeginTabItem("Shells"))
            {
                DrawShellsTab(shells, registry);
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}
