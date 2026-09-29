#include "World/TextureCoordinates.h"

#include <glm/geometric.hpp>

#include <cassert>

namespace Abomination::World
{
    namespace
    {
        // A scale of 0 would divide by zero; TrenchBroom never writes it, but a hand-edited map could.
        double MakeValidScale(double scale)
        {
            return scale != 0.0 ? scale : 1.0;
        }
    }

    glm::vec2 CalculateTextureCoordinates(const MapFace& face, const glm::dvec3& point, glm::ivec2 textureSize)
    {
        // The texels are divided by the texture size below. Every texture has a size (a missing one gets the 8x8 fallback).
        assert(textureSize.x > 0 && textureSize.y > 0);

        const double texelU = glm::dot(point, face.textureUAxis) / MakeValidScale(face.textureScaleU) + face.textureOffsetU;
        const double texelV = glm::dot(point, face.textureVAxis) / MakeValidScale(face.textureScaleV) + face.textureOffsetV;

        // Calculated in doubles like the geometry; the result is small (a few repeats of the texture), so float is enough.
        return {
            static_cast<float>(texelU / textureSize.x),
            static_cast<float>(-texelV / textureSize.y),
        };
    }
}
