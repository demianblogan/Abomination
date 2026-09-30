#include "UI/WeaponWindow.h"

#include "Gameplay/ViewModelMotion.h"
#include "Gameplay/ViewRecoil.h"
#include "Gameplay/Weapon.h"
#include "UI/UIScale.h"

#include <glm/trigonometric.hpp>
#include <imgui.h>

#include <cmath>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time next to the View Model window.
        constexpr ImVec2 InitialPosition(560.0f, 620.0f);

        // The width of the sliders in pixels at 100% scale.
        constexpr float SliderWidth = 180.0f;

        // A slider for an angle in degrees, stored in radians.
        void DrawDegreeSlider(const char* label, float& radians, float minimumDegrees, float maximumDegrees,
                              const char* tooltip)
        {
            float degrees = glm::degrees(radians);
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            if (ImGui::SliderFloat(label, &degrees, minimumDegrees, maximumDegrees, "%.1f deg"))
                radians = glm::radians(degrees);
            ImGui::SetItemTooltip("%s", tooltip);
        }

        // The sliders of one kind of hit marker. PushID keeps the equal labels of the hit and the kill marker apart for
        // ImGui, which makes the ID of an item from its label.
        void DrawHitMarkerSettings(const char* title, Gameplay::HitMarkerSettings& marker)
        {
            ImGui::SeparatorText(title);
            ImGui::PushID(title);

            ImGui::ColorEdit4("Color", &marker.color.r, ImGuiColorEditFlags_NoInputs);
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            ImGui::SliderFloat("Line length", &marker.lineLength, 1.0f, 30.0f, "%.1f px");
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            ImGui::SliderFloat("Gap from circle", &marker.startGap, -20.0f, 30.0f, "%.1f px");
            ImGui::SetItemTooltip("Where the lines start, from the circle. Negative: inside it.");
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            ImGui::SliderFloat("Travel", &marker.travelDistance, -30.0f, 30.0f, "%.1f px");
            ImGui::SetItemTooltip("How far the lines move while they fade. Positive: outwards, negative: inwards.");
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            ImGui::SliderFloat("Duration", &marker.duration, 0.05f, 1.0f, "%.2f s");
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            ImGui::SliderFloat("Thickness", &marker.thickness, 0.5f, 6.0f, "%.1f px");

            ImGui::PopID();
        }
    }

    void DrawWeaponWindow(bool* isOpen, Gameplay::Weapon& weapon, Gameplay::ViewRecoil& viewRecoil,
                          Gameplay::ViewModelMotionSettings& viewModelMotion)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Weapon", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        Gameplay::WeaponSettings& settings = weapon.settings;
        ImGui::SeparatorText("Shot");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderInt("Pellets", &settings.pelletCount, 1, 20);
        ImGui::SetItemTooltip("How many pellets one shot sends.");

        DrawDegreeSlider("Spread", settings.spreadAngle, 0.0f, 15.0f,
                         "How far a pellet may fly off the middle of the screen (half the angle of the cone).\n"
                         "Smaller: a tighter group that hits farther away.");

        // How wide the pellets spread at 10 m, to picture the spread angle.
        const float widthAtTenMeters = 2.0f * 10.0f * std::tan(settings.spreadAngle);
        ImGui::Text("Group at 10 m:  %.2f m wide", widthAtTenMeters);

        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Range", &settings.range, 5.0f, 200.0f, "%.0f m");
        ImGui::SetItemTooltip("Pellets fly this far and hit nothing beyond it.");

        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Time between shots", &settings.timeBetweenShots, 0.1f, 2.0f, "%.2f s");
        ImGui::SetItemTooltip("The rhythm of the weapon: holding Fire shoots again after this time.");

        ImGui::Checkbox("Show pellet lines", &weapon.areShotLinesVisible);
        ImGui::SetItemTooltip("Lines of the pellets of the last shot for 2 seconds: red with a box where they hit,\n"
                              "gray where they flew into nothing. Best seen from the free-fly camera (F2).");

        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Damage per pellet", &settings.damagePerPellet, 0.0f, 50.0f, "%.1f");
        ImGui::SetItemTooltip("Health one pellet takes. A target dummy has 100.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Knockback per pellet", &settings.knockbackPerPellet, 0.0f, 5.0f, "%.2f m/s");
        ImGui::SetItemTooltip("How hard one pellet pushes what it hits away from the shooter.");

        Gameplay::CrosshairSettings& crosshair = weapon.crosshair;
        ImGui::SeparatorText("Crosshair");
        ImGui::ColorEdit4("Crosshair color", &crosshair.color.r, ImGuiColorEditFlags_NoInputs);
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Circle thickness", &crosshair.circleThickness, 0.5f, 5.0f, "%.1f px");
        ImGui::SetItemTooltip("The circle is as wide as the spread: every pellet lands inside it.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Dot radius", &crosshair.dotRadius, 0.0f, 5.0f, "%.1f px");

        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Pulse kick", &crosshair.pulseKick, 0.0f, 20.0f, "%.1f");
        ImGui::SetItemTooltip("How much a shot widens the circle for a moment. 0 turns the pulse off.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Pulse stiffness", &crosshair.pulseStiffness, 20.0f, 800.0f, "%.0f");
        ImGui::SetItemTooltip("How fast the circle comes back. Bigger: quicker and smaller.");

        DrawHitMarkerSettings("Hit marker", crosshair.hitMarker);
        DrawHitMarkerSettings("Kill marker", crosshair.killMarker);

        ImGui::SeparatorText("Recoil of the view");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("View kick", &viewRecoil.kick, 0.0f, 2.0f, "%.2f");
        ImGui::SetItemTooltip("How hard a shot jerks the view up. Only the picture moves: the aim stays.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("View stiffness", &viewRecoil.springStiffness, 20.0f, 500.0f, "%.0f");
        ImGui::SetItemTooltip("How fast the view comes back down. Bigger: quicker and smaller kicks.");

        ImGui::SeparatorText("Recoil of the weapon in the hands");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Kick back", &viewModelMotion.recoilKickBack, 0.0f, 5.0f, "%.2f m/s");
        ImGui::SetItemTooltip("How hard a shot pushes the weapon back towards the eyes.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Kick up", &viewModelMotion.recoilKickUp, 0.0f, 5.0f, "%.2f rad/s");
        ImGui::SetItemTooltip("How hard a shot turns the muzzle up.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Weapon stiffness", &viewModelMotion.recoilStiffness, 20.0f, 500.0f, "%.0f");
        ImGui::SetItemTooltip("How fast the weapon comes back. Bigger: quicker and smaller kicks.");

        if (ImGui::Button("Reset"))
        {
            weapon.settings = Gameplay::WeaponSettings{};
            weapon.crosshair = Gameplay::CrosshairSettings{};

            const Gameplay::ViewRecoil recoilDefaults;
            viewRecoil.kick = recoilDefaults.kick;
            viewRecoil.springStiffness = recoilDefaults.springStiffness;

            const Gameplay::ViewModelMotionSettings motionDefaults;
            viewModelMotion.recoilKickBack = motionDefaults.recoilKickBack;
            viewModelMotion.recoilKickUp = motionDefaults.recoilKickUp;
            viewModelMotion.recoilStiffness = motionDefaults.recoilStiffness;
        }

        ImGui::End();
    }
}
