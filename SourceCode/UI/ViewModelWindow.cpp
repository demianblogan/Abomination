#include "UI/ViewModelWindow.h"

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

    void DrawViewModelWindow(bool* isOpen, Gameplay::ViewModel& viewModel)
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
        DrawCentimeterSlider("Right", viewModel.offset.x, 0.0f, 50.0f,
                             "How far to the side the middle of the weapon is. For the left side it goes to the left,\n"
                             "in the center it is ignored.");
        DrawCentimeterSlider("Up", viewModel.offset.y, -50.0f, 10.0f, "Negative: below the eyes.");
        DrawCentimeterSlider("Forward", viewModel.offset.z, -100.0f, 0.0f,
                             "Negative: in front of the eyes. The middle of the shotgun: it is 1.1 m long, so at -42 cm\n"
                             "its muzzle is about 1 m in front of the eyes and its stock just behind them.");

        ImGui::SeparatorText("Lens");
        float fovDegrees = glm::degrees(viewModel.verticalFOV);
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        if (ImGui::SliderFloat("Vertical FOV", &fovDegrees, 30.0f, 90.0f, "%.0f deg"))
            viewModel.verticalFOV = glm::radians(fovDegrees);
        ImGui::SetItemTooltip("The field of view the weapon alone is drawn with. Smaller: the weapon looks bigger and\n"
                              "flatter; larger: smaller and more stretched in depth. The world is not affected.");

        if (ImGui::Button("Reset"))
        {
            // Only the placement goes back to the defaults; the model and the shader stay.
            const Gameplay::ViewModel defaults;
            viewModel.offset = defaults.offset;
            viewModel.side = defaults.side;
            viewModel.verticalFOV = defaults.verticalFOV;
        }

        ImGui::End();
    }
}
