#pragma once

#include "World/MapData.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Abomination::World
{
    // The texture coordinates of a point of a face, from the texture axes the Valve 220 format stores for every face.
    //
    // The position of the point along each texture axis, in world units, is its dot product with the axis. Divided by
    // the scale (world units per texel) it becomes texels, the offset (in texels) moves the texture along the face, and
    // divided by the size of the texture it becomes the coordinate OpenGL expects: 0 to 1 across the texture once,
    // 1 to 2 the second time it repeats, and so on.
    //
    // V is negated at the end: in the .map format V grows down the image (like rows of pixels), while in OpenGL it
    // grows up (Core::LoadImageFile stores the bottom row first).
    //
    // point is in map units and axes, like the face. textureSize is the size of the texture in texels.
    [[nodiscard]] glm::vec2 CalculateTextureCoordinates(const MapFace& face, const glm::dvec3& point, glm::ivec2 textureSize);

    // The tangent of a face for normal maps, in game axes (see Renderer::MeshVertex::tangent): the direction in which the U
    // texture coordinate of CalculateTextureCoordinates grows, laid on the face, and in w the sign that makes
    // cross(normal, tangent) the direction in which V grows. It comes straight from the texture axes of the face, so it is
    // exact, and the same for every vertex of the face. normal is the normal of the face in game axes (length 1).
    [[nodiscard]] glm::vec4 CalculateTangent(const MapFace& face, const glm::vec3& normal);
}
