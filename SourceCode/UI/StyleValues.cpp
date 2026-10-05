#include "UI/StyleValues.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>

#include <glm/common.hpp>

#include <cmath>
#include <format>

namespace Abomination::UI
{
    std::string ToRCSSColor(const glm::vec4& color)
    {
        // Every channel from 0..1 to a whole number 0..255, written as two hex digits.
        const auto channel = [](float value)
        {
            return static_cast<int>(std::lround(glm::clamp(value, 0.0f, 1.0f) * 255.0f));
        };
        return std::format("#{:02x}{:02x}{:02x}{:02x}", channel(color.r), channel(color.g), channel(color.b),
                           channel(color.a));
    }

    std::string ToPixels(float pixels)
    {
        return std::format("{:.2f}px", pixels);
    }

    std::string ToOpacity(float opacity)
    {
        return std::format("{:.3f}", glm::clamp(opacity, 0.0f, 1.0f));
    }

    void SetOpacity(Rml::Element* element, float opacity)
    {
        if (element != nullptr)
            element->SetProperty("opacity", ToOpacity(opacity));
    }

    void SetVisible(Rml::Element* element, bool isVisible)
    {
        if (element != nullptr)
            element->SetProperty("visibility", isVisible ? "visible" : "hidden");
    }

    void SetShown(Rml::ElementDocument& document, bool isShown)
    {
        if (isShown == document.IsVisible())
            return;
        if (isShown)
            document.Show();
        else
            document.Hide();
    }
}
