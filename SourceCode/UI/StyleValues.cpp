#include "UI/StyleValues.h"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/PropertyDictionary.h>
#include <RmlUi/Core/StyleSheetSpecification.h>

#include <glm/common.hpp>

#include <algorithm>
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

    void SetProperty(Rml::Element* element, const std::string& name, const std::string& value)
    {
        if (element == nullptr)
            return;

        // The value as RmlUi will keep it. A shorthand ("border-width") becomes several properties (the four sides).
        // A value RmlUi cannot parse is passed on as it is, so RmlUi reports it in the log.
        Rml::PropertyDictionary parsed;
        if (!Rml::StyleSheetSpecification::ParsePropertyDeclaration(parsed, name, value))
        {
            element->SetProperty(name, value);
            return;
        }

        // Set by the element itself (not inherited or from a style sheet) with exactly these values: nothing to change.
        // A transform is never the same (its values are shared pointers), but changing it does not rebuild geometry.
        const bool isSame = std::ranges::all_of(parsed.GetProperties(), [element](const auto& entry)
        {
            const Rml::Property* current = element->GetLocalProperty(entry.first);
            return current != nullptr && *current == entry.second;
        });
        if (!isSame)
            element->SetProperty(name, value);
    }

    void SetOpacity(Rml::Element* element, float opacity)
    {
        SetProperty(element, "opacity", ToOpacity(opacity));
    }

    void SetVisible(Rml::Element* element, bool isVisible)
    {
        SetProperty(element, "visibility", isVisible ? "visible" : "hidden");
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
