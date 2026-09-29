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
