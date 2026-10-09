#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// Conversions between sRGB numbers and linear values (see Documentation/ARCHITECTURE.md, section 6, linear lighting).
// sRGB is how image files, color pickers and the screen store a color: the numbers are not proportional to the amount
// of light (0.5 is about 21% of white), so that the 256 steps of 8 bits are spent where the eye sees differences.
// Light is added and multiplied in linear values, which are proportional to it. The shaders have the same formulas
// (ConvertSRGBToLinear in Sprite.frag, ConvertLinearToSRGB in Present.frag).
namespace Abomination::Renderer
{
    // One channel (0..1) of an sRGB color as a linear value (0..1), by the exact formula of the sRGB standard: a short
    // straight piece near black, then a power of 2.4.
    [[nodiscard]] float ConvertSRGBToLinear(float value);

    // The opposite: a linear value (0..1) as an sRGB number (0..1).
    [[nodiscard]] float ConvertLinearToSRGB(float value);

    // A color given in sRGB numbers (as a designer picks it) as linear values. Alpha is not a color: it is kept as it is.
    [[nodiscard]] glm::vec3 ConvertSRGBToLinear(const glm::vec3& color);
    [[nodiscard]] glm::vec4 ConvertSRGBToLinear(const glm::vec4& color);

    // Linear values as an sRGB color, to show them in a color picker or as debug lines.
    [[nodiscard]] glm::vec3 ConvertLinearToSRGB(const glm::vec3& color);
}
