#pragma once

// Controls the debug windows share, so every window has sliders of the same width and the tuning windows (Movement,
// Weapon View Model, Weapon, Effects) are one line per value.
namespace Abomination::UI
{
    // The width of a slider in pixels at 100% UI scale.
    inline constexpr float SliderWidth = 180.0f;

    // A slider for a value, format like printf ("%.2f m/s"); tooltip (optional) is shown when the mouse is over it.
    // Returns whether the value changed. Every slider below works the same way.
    bool DrawSlider(const char* label, float& value, float minimum, float maximum, const char* format,
                    const char* tooltip = nullptr);

    bool DrawIntSlider(const char* label, int& value, int minimum, int maximum, const char* tooltip = nullptr);

    // A distance stored in meters, shown in centimeters: easier to read and to set exactly for small distances.
    bool DrawCentimeterSlider(const char* label, float& meters, float minimumCentimeters, float maximumCentimeters,
                              const char* tooltip = nullptr);

    // An angle stored in radians, shown in degrees.
    bool DrawDegreeSlider(const char* label, float& radians, float minimumDegrees, float maximumDegrees,
                          const char* format = "%.1f deg", const char* tooltip = nullptr);
}
