#include "World/TextureCoordinates.h"

#include "World/MapCoordinates.h"

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

    glm::vec4 CalculateTangent(const MapFace& face, const glm::vec3& normal)
    {
        // Where the texture coordinates grow, in map axes: U along the U axis, V against the V axis (V is negated in
        // CalculateTextureCoordinates). A negative scale turns the texture around, and the direction with it. Then into
        // game axes; ConvertMapPosition also shrinks to meters, which does not change a direction.
        const glm::vec3 uDirection = ConvertMapPosition(face.textureUAxis / MakeValidScale(face.textureScaleU));
        const glm::vec3 vDirection = ConvertMapPosition(-face.textureVAxis / MakeValidScale(face.textureScaleV));

        // The U axis may not lie exactly on the face (TrenchBroom keeps the axes of a face it was rotated from): the part
        // along the normal is removed (Gram-Schmidt), so the tangent lies on the surface.
        const glm::vec3 alongFace = uDirection - normal * glm::dot(uDirection, normal);
        const float length = glm::length(alongFace);

        // A U axis along the normal would give no direction (a texture seen edge-on): any tangent will do then. Compared
        // with the length of the axis itself, which is small (map units become meters, divided by the scale).
        if (length <= glm::length(uDirection) * 1e-4f)
            return {1.0f, 0.0f, 0.0f, 1.0f};
        const glm::vec3 tangent = alongFace / length;

        // cross(normal, tangent) is one of the two directions across the tangent; w says whether V grows along it or
        // against it (a mirrored texture).
        const float handedness = glm::dot(glm::cross(normal, tangent), vDirection) < 0.0f ? -1.0f : 1.0f;

        return {tangent, handedness};
    }
}
