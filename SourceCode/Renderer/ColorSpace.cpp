#include "Renderer/ColorSpace.h"

#include <cmath>

namespace Abomination::Renderer
{
    float ConvertSRGBToLinear(float value)
    {
        // Below 0.04045 the curve is a straight line (a pure power would be too steep near black to invert precisely).
        if (value <= 0.04045f)
            return value / 12.92f;

        return std::pow((value + 0.055f) / 1.055f, 2.4f);
    }

    float ConvertLinearToSRGB(float value)
    {
        if (value <= 0.0031308f)
            return value * 12.92f;

        return 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
    }

    glm::vec3 ConvertSRGBToLinear(const glm::vec3& color)
    {
        return {ConvertSRGBToLinear(color.r), ConvertSRGBToLinear(color.g), ConvertSRGBToLinear(color.b)};
    }

    glm::vec4 ConvertSRGBToLinear(const glm::vec4& color)
    {
        return {ConvertSRGBToLinear(glm::vec3(color)), color.a};
    }
}
