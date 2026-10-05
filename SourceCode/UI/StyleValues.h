#pragma once

#include <glm/vec4.hpp>

#include <string>

namespace Rml
{
    class Element;
    class ElementDocument;
}

// Values for the properties of RmlUi elements. Element::SetProperty takes a value written as in a style sheet ("0.5",
// "12.00px", "#ff0000ff"), so numbers are turned into such text here, one way for the whole game interface.
namespace Abomination::UI
{
    // A color as RCSS writes it: #rrggbbaa.
    [[nodiscard]] std::string ToRCSSColor(const glm::vec4& color);

    // A length in pixels: "12.50px".
    [[nodiscard]] std::string ToPixels(float pixels);

    // An opacity, clamped to 0..1: "0.500".
    [[nodiscard]] std::string ToOpacity(float opacity);

    // Sets a property of an element, but only when its value changes (nothing for a missing element). RmlUi takes every
    // Element::SetProperty as a change, even to the same value: it lays the element out again and builds its geometry
    // anew (new OpenGL buffers, the old ones deleted). The HUD sets its properties every frame, so this cut the cost
    // of drawing it (see Tools/Profiling). The value is compared after RmlUi has parsed it, so "1.50px" and "1.5px" are
    // the same.
    void SetProperty(Rml::Element* element, const std::string& name, const std::string& value);

    // Sets the opacity of an element (nothing for a missing one).
    void SetOpacity(Rml::Element* element, float opacity);

    // Sets an element visible or not (nothing for a missing one); a hidden element is not drawn and does not take the
    // mouse.
    void SetVisible(Rml::Element* element, bool isVisible);

    // Shows or hides a whole document, only when that changes it.
    void SetShown(Rml::ElementDocument& document, bool isShown);
}
