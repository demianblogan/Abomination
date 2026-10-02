#include "UI/Windows/WeaponWindow.h"

#include "Gameplay/Weapons/ViewRecoil.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Gameplay/Weapons/WeaponViewModelMotion.h"
#include "UI/UIScale.h"
#include "UI/Widgets.h"

#include <imgui.h>

#include <cmath>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time next to the Weapon View Model window.
        constexpr ImVec2 InitialPosition(560.0f, 620.0f);

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
    }

    void DrawWeaponWindow(bool* isOpen, Gameplay::Weapon& weapon, Gameplay::ViewRecoil& viewRecoil,
                          Gameplay::WeaponViewModelMotionSettings& weaponViewModelMotion)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Weapon", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        Gameplay::WeaponSettings& settings = weapon.settings;
        ImGui::SeparatorText("Shot");
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

        ImGui::Checkbox("Show pellet lines", &weapon.areShotLinesVisible);
        ImGui::SetItemTooltip("Lines of the pellets of the last shot for 2 seconds: yellow where they hit a character, red\n"
                              "with a box where they hit a wall, gray where they flew into nothing. Best seen from the\n"
                              "free-fly camera (F2).");

        DrawSlider("Damage per pellet", settings.damagePerPellet, 0.0f, 50.0f, "%.1f",
                   "Health one pellet takes. A target dummy has 100.");
        DrawSlider("Knockback per pellet", settings.knockbackPerPellet, 0.0f, 5.0f, "%.2f m/s",
                   "How hard one pellet pushes what it hits away from the shooter.");

        Gameplay::CrosshairSettings& crosshair = weapon.crosshair;
        ImGui::SeparatorText("Crosshair");
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

        ImGui::SeparatorText("Recoil of the view");
        DrawSlider("View kick", viewRecoil.kick, 0.0f, 2.0f, "%.2f",
                   "How hard a shot jerks the view up. Only the picture moves: the aim stays.");
        DrawSlider("View stiffness", viewRecoil.springStiffness, 20.0f, 500.0f, "%.0f",
                   "How fast the view comes back down. Bigger: quicker and smaller kicks.");

        ImGui::SeparatorText("Recoil of the weapon in the hands");
        DrawSlider("Kick back", weaponViewModelMotion.recoilKickBack, 0.0f, 5.0f, "%.2f m/s",
                   "How hard a shot pushes the weapon back towards the eyes.");
        DrawSlider("Kick up", weaponViewModelMotion.recoilKickUp, 0.0f, 5.0f, "%.2f rad/s",
                   "How hard a shot turns the muzzle up.");
        DrawSlider("Weapon stiffness", weaponViewModelMotion.recoilStiffness, 20.0f, 500.0f, "%.0f",
                   "How fast the weapon comes back. Bigger: quicker and smaller kicks.");

        if (ImGui::Button("Reset"))
        {
            weapon.settings = Gameplay::WeaponSettings{};
            weapon.crosshair = Gameplay::CrosshairSettings{};

            const Gameplay::ViewRecoil recoilDefaults;
            viewRecoil.kick = recoilDefaults.kick;
            viewRecoil.springStiffness = recoilDefaults.springStiffness;

            const Gameplay::WeaponViewModelMotionSettings motionDefaults;
            weaponViewModelMotion.recoilKickBack = motionDefaults.recoilKickBack;
            weaponViewModelMotion.recoilKickUp = motionDefaults.recoilKickUp;
            weaponViewModelMotion.recoilStiffness = motionDefaults.recoilStiffness;
        }

        ImGui::End();
    }
}
