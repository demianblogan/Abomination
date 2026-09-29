#include "UI/ViewModelWindow.h"

#include "Gameplay/LandingDip.h"
#include "Gameplay/ViewModel.h"
#include "UI/UIScale.h"

#include <glm/trigonometric.hpp>
#include <imgui.h>

#include <cstddef>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time at the right edge, below the Collisions window.
        constexpr ImVec2 InitialPosition(900.0f, 620.0f);

        // The width of the sliders in pixels at 100% scale.
        constexpr float SliderWidth = 180.0f;

        // A slider for a distance in centimeters, stored in meters: centimeters are easier to read and to set exactly.
        void DrawCentimeterSlider(const char* label, float& meters, float minimumCentimeters, float maximumCentimeters,
                                  const char* tooltip)
        {
            float centimeters = meters * 100.0f;
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            if (ImGui::SliderFloat(label, &centimeters, minimumCentimeters, maximumCentimeters, "%.1f cm"))
                meters = centimeters / 100.0f;
            ImGui::SetItemTooltip("%s", tooltip);
        }
    }

    void DrawViewModelWindow(bool* isOpen, Gameplay::ViewModel& viewModel, Gameplay::LandingDip& landingDip)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("View Model", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        ImGui::SeparatorText("Side");
        for (std::size_t index = 0; index < Gameplay::ViewModelSideNames.size(); ++index)
        {
            if (index > 0)
                ImGui::SameLine();

            const auto side = static_cast<Gameplay::ViewModelSide>(index);
            if (ImGui::RadioButton(Gameplay::ViewModelSideNames[index].data(), viewModel.side == side))
                viewModel.side = side;
        }

        ImGui::SeparatorText("Position from the eyes");
        // Not called "Right": ImGui makes the ID of an item from its label, and the radio button above is "Right" already.
        DrawCentimeterSlider("Sideways", viewModel.offset.x, 0.0f, 50.0f,
                             "How far to the side the middle of the weapon is. For the left side it goes to the left,\n"
                             "in the center it is ignored.");
        DrawCentimeterSlider("Up", viewModel.offset.y, -50.0f, 10.0f, "Negative: below the eyes.");
        DrawCentimeterSlider("Forward", viewModel.offset.z, -100.0f, 0.0f,
                             "Negative: in front of the eyes. The middle of the shotgun: it is 1.1 m long, so at -35 cm\n"
                             "its muzzle is about 90 cm in front of the eyes and its stock behind them.");

        ImGui::SeparatorText("Lens");
        float fovDegrees = glm::degrees(viewModel.verticalFOV);
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        if (ImGui::SliderFloat("Vertical FOV", &fovDegrees, 30.0f, 90.0f, "%.0f deg"))
            viewModel.verticalFOV = glm::radians(fovDegrees);
        ImGui::SetItemTooltip("The field of view the weapon alone is drawn with. Smaller: the weapon looks bigger and\n"
                              "flatter; larger: smaller and more stretched in depth. The world is not affected.");

        Gameplay::ViewModelMotionSettings& motion = viewModel.motionSettings;
        ImGui::SeparatorText("Bob (walking)");
        DrawCentimeterSlider("Bob amount", motion.bobAmount, 0.0f, 5.0f,
                             "How far the weapon swings to the sides at full speed. 0 turns the bob off.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Stride length", &motion.bobStrideLength, 0.5f, 5.0f, "%.2f m");
        ImGui::SetItemTooltip("How far the player walks during one whole swing (two steps). Shorter: faster swings.");

        ImGui::SeparatorText("Sway (turning)");
        DrawCentimeterSlider("Sway amount", motion.swayAmount, 0.0f, 10.0f,
                             "How far the weapon lags per radian (57 degrees) the view turns. 0 turns the sway off.");
        DrawCentimeterSlider("Sway maximum", motion.swayMaximum, 0.0f, 10.0f, "The farthest the weapon ever lags.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Sway return", &motion.swayReturnRate, 1.0f, 30.0f, "%.1f /s");
        ImGui::SetItemTooltip("How fast the weapon catches up with the view. Bigger: quicker, stiffer.");

        ImGui::SeparatorText("Inertia (jumping, landing)");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Jump kick", &motion.jumpKick, 0.0f, 1.0f, "%.2f m/s");
        ImGui::SetItemTooltip("How hard a jump pushes the weapon down. 0 turns it off.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Landing kick", &motion.landingKickPerFallSpeed, 0.0f, 0.1f, "%.3f");
        ImGui::SetItemTooltip("How hard a landing pushes the weapon down, per m/s of the fall (a jump lands at 8.4 m/s).");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Spring stiffness", &motion.springStiffness, 20.0f, 500.0f, "%.0f");
        ImGui::SetItemTooltip("How fast the weapon comes back after a kick. Bigger: quicker and smaller dips.");

        ImGui::SeparatorText("Camera landing dip");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Dip kick", &landingDip.kickPerFallSpeed, 0.0f, 0.2f, "%.3f");
        ImGui::SetItemTooltip("How far the view dips after a landing, per m/s of the fall. 0 turns it off.");
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        ImGui::SliderFloat("Dip stiffness", &landingDip.springStiffness, 20.0f, 500.0f, "%.0f");
        ImGui::SetItemTooltip("How fast the view comes back up. Bigger: quicker and smaller dips.");

        if (ImGui::Button("Reset"))
        {
            // Only the settings go back to the defaults; the model, the shader and the current motion stay.
            const Gameplay::ViewModel defaults;
            viewModel.offset = defaults.offset;
            viewModel.side = defaults.side;
            viewModel.verticalFOV = defaults.verticalFOV;
            viewModel.motionSettings = defaults.motionSettings;

            const Gameplay::LandingDip dipDefaults;
            landingDip.kickPerFallSpeed = dipDefaults.kickPerFallSpeed;
            landingDip.springStiffness = dipDefaults.springStiffness;
        }

        ImGui::End();
    }
}
