#include "UI/Widgets.h"

#include "UI/UIScale.h"

#include <glm/trigonometric.hpp>
#include <imgui.h>

namespace Abomination::UI
{
    namespace
    {
        void ShowTooltip(const char* tooltip)
        {
            if (tooltip != nullptr)
                ImGui::SetItemTooltip("%s", tooltip);
        }
    }

    bool DrawSlider(const char* label, float& value, float minimum, float maximum, const char* format, const char* tooltip)
    {
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        const bool hasChanged = ImGui::SliderFloat(label, &value, minimum, maximum, format);
        ShowTooltip(tooltip);
        return hasChanged;
    }

    bool DrawIntSlider(const char* label, int& value, int minimum, int maximum, const char* tooltip)
    {
        ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
        const bool hasChanged = ImGui::SliderInt(label, &value, minimum, maximum);
        ShowTooltip(tooltip);
        return hasChanged;
    }

    bool DrawCentimeterSlider(const char* label, float& meters, float minimumCentimeters, float maximumCentimeters,
                              const char* tooltip)
    {
        float centimeters = meters * 100.0f;
        if (!DrawSlider(label, centimeters, minimumCentimeters, maximumCentimeters, "%.1f cm", tooltip))
            return false;

        meters = centimeters / 100.0f;
        return true;
    }

    bool DrawDegreeSlider(const char* label, float& radians, float minimumDegrees, float maximumDegrees, const char* format,
                          const char* tooltip)
    {
        float degrees = glm::degrees(radians);
        if (!DrawSlider(label, degrees, minimumDegrees, maximumDegrees, format, tooltip))
            return false;

        radians = glm::radians(degrees);
        return true;
    }
}
