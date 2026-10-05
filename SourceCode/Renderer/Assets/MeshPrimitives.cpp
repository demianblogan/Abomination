#include "Renderer/Assets/MeshPrimitives.h"

#include "Renderer/Assets/MeshTangents.h"

#include <glm/vec3.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

namespace Abomination::Renderer
{
    MeshData CreateCubeMeshData()
    {
        // A corner shared by 3 faces needs different texture coordinates on each of them, so every face has its own
        // 4 vertices (24 in total) instead of sharing the 8 corners.
        // On every face the vertices go counter-clockwise when looked at from OUTSIDE the cube, starting at the
        // bottom-left corner: this is how OpenGL recognizes the front side of a triangle (face culling).
        MeshData data;
        data.vertices = {
            // Front (+Z)
            {.position = {-0.5f, -0.5f, 0.5f}, .texCoord = {0.0f, 0.0f}},
            {.position = {0.5f, -0.5f, 0.5f}, .texCoord = {1.0f, 0.0f}},
            {.position = {0.5f, 0.5f, 0.5f}, .texCoord = {1.0f, 1.0f}},
            {.position = {-0.5f, 0.5f, 0.5f}, .texCoord = {0.0f, 1.0f}},
            // Back (-Z)
            {.position = {0.5f, -0.5f, -0.5f}, .texCoord = {0.0f, 0.0f}},
            {.position = {-0.5f, -0.5f, -0.5f}, .texCoord = {1.0f, 0.0f}},
            {.position = {-0.5f, 0.5f, -0.5f}, .texCoord = {1.0f, 1.0f}},
            {.position = {0.5f, 0.5f, -0.5f}, .texCoord = {0.0f, 1.0f}},
            // Right (+X)
            {.position = {0.5f, -0.5f, 0.5f}, .texCoord = {0.0f, 0.0f}},
            {.position = {0.5f, -0.5f, -0.5f}, .texCoord = {1.0f, 0.0f}},
            {.position = {0.5f, 0.5f, -0.5f}, .texCoord = {1.0f, 1.0f}},
            {.position = {0.5f, 0.5f, 0.5f}, .texCoord = {0.0f, 1.0f}},
            // Left (-X)
            {.position = {-0.5f, -0.5f, -0.5f}, .texCoord = {0.0f, 0.0f}},
            {.position = {-0.5f, -0.5f, 0.5f}, .texCoord = {1.0f, 0.0f}},
            {.position = {-0.5f, 0.5f, 0.5f}, .texCoord = {1.0f, 1.0f}},
            {.position = {-0.5f, 0.5f, -0.5f}, .texCoord = {0.0f, 1.0f}},
            // Top (+Y)
            {.position = {-0.5f, 0.5f, 0.5f}, .texCoord = {0.0f, 0.0f}},
            {.position = {0.5f, 0.5f, 0.5f}, .texCoord = {1.0f, 0.0f}},
            {.position = {0.5f, 0.5f, -0.5f}, .texCoord = {1.0f, 1.0f}},
            {.position = {-0.5f, 0.5f, -0.5f}, .texCoord = {0.0f, 1.0f}},
            // Bottom (-Y)
            {.position = {-0.5f, -0.5f, -0.5f}, .texCoord = {0.0f, 0.0f}},
            {.position = {0.5f, -0.5f, -0.5f}, .texCoord = {1.0f, 0.0f}},
            {.position = {0.5f, -0.5f, 0.5f}, .texCoord = {1.0f, 1.0f}},
            {.position = {-0.5f, -0.5f, 0.5f}, .texCoord = {0.0f, 1.0f}},
        };

        // All 4 vertices of a face share the direction the face looks in, in the same order as the faces above.
        constexpr std::array<glm::vec3, 6> FaceNormals{
            glm::vec3(0.0f, 0.0f, 1.0f),  // Front
            glm::vec3(0.0f, 0.0f, -1.0f), // Back
            glm::vec3(1.0f, 0.0f, 0.0f),  // Right
            glm::vec3(-1.0f, 0.0f, 0.0f), // Left
            glm::vec3(0.0f, 1.0f, 0.0f),  // Top
            glm::vec3(0.0f, -1.0f, 0.0f), // Bottom
        };
        for (std::size_t vertexIndex = 0; vertexIndex < data.vertices.size(); ++vertexIndex)
            data.vertices[vertexIndex].normal = FaceNormals[vertexIndex / 4];

        // Two triangles per face: face N uses vertices 4N .. 4N+3 as (0, 1, 2) and (2, 3, 0), counter-clockwise.
        constexpr int FaceCount = 6;
        data.indices.reserve(FaceCount * 6);
        for (std::uint32_t face = 0; face < FaceCount; ++face)
        {
            const std::uint32_t first = face * 4;
            data.indices.insert(data.indices.end(), {first, first + 1, first + 2, first + 2, first + 3, first});
        }

        // The axes of the texture for normal maps (see GenerateTangents). A mesh made here always has triangles.
        [[maybe_unused]] const bool areTangentsGenerated = GenerateTangents(data);
        assert(areTangentsGenerated);

        return data;
    }

    MeshData CreateCylinderMeshData(int sideCount, float radius, float length)
    {
        MeshData data;
        const float halfLength = length * 0.5f;

        // The sides: a column of 2 vertices (bottom, top) at every corner of the polygon, and one more column at the end,
        // at the same place as the first but with u = 1: the texture ends there, so the seam cannot share vertices.
        // The normals point straight out from the axis, the same at the bottom and the top: the sides are shaded
        // smoothly around, which makes 8 sides look rounder.
        for (int corner = 0; corner <= sideCount; ++corner)
        {
            const float u = static_cast<float>(corner) / static_cast<float>(sideCount);
            const float angle = u * 2.0f * std::numbers::pi_v<float>;

            // Around Y from +X towards -Z: counter-clockwise when looked at from above.
            const glm::vec3 outward(std::cos(angle), 0.0f, -std::sin(angle));
            data.vertices.push_back({.position = outward * radius + glm::vec3(0.0f, -halfLength, 0.0f),
                                     .texCoord = {u, 0.0f}, .normal = outward});
            data.vertices.push_back({.position = outward * radius + glm::vec3(0.0f, halfLength, 0.0f),
                                     .texCoord = {u, 1.0f}, .normal = outward});
        }

        // Two triangles per side, counter-clockwise from outside: bottom-left, bottom-right, top-right and back.
        for (std::uint32_t side = 0; side < static_cast<std::uint32_t>(sideCount); ++side)
        {
            const std::uint32_t bottomLeft = side * 2;
            const std::uint32_t topLeft = bottomLeft + 1;
            const std::uint32_t bottomRight = bottomLeft + 2;
            const std::uint32_t topRight = bottomLeft + 3;
            data.indices.insert(data.indices.end(), {bottomLeft, bottomRight, topRight, topRight, topLeft, bottomLeft});
        }

        // The ends: a fan of triangles from the middle. Every vertex of an end takes one point of the texture, just inside
        // the bottom or the top row (the middle of the edge texel would be exact only for one texture size).
        const auto addEnd = [&](float y, float v, const glm::vec3& normal)
        {
            const auto middle = static_cast<std::uint32_t>(data.vertices.size());
            data.vertices.push_back({.position = {0.0f, y, 0.0f}, .texCoord = {0.5f, v}, .normal = normal});
            for (int corner = 0; corner < sideCount; ++corner)
            {
                const float u = static_cast<float>(corner) / static_cast<float>(sideCount);
                const float angle = u * 2.0f * std::numbers::pi_v<float>;
                data.vertices.push_back({.position = {std::cos(angle) * radius, y, -std::sin(angle) * radius},
                                         .texCoord = {0.5f, v}, .normal = normal});
            }

            // Looked at from above the corners go counter-clockwise: so the top end uses them in this order, and the
            // bottom one, seen from below, the other way around.
            for (std::uint32_t corner = 0; corner < static_cast<std::uint32_t>(sideCount); ++corner)
            {
                const std::uint32_t current = middle + 1 + corner;
                const std::uint32_t next = middle + 1 + (corner + 1) % static_cast<std::uint32_t>(sideCount);
                if (normal.y > 0.0f)
                    data.indices.insert(data.indices.end(), {middle, current, next});
                else
                    data.indices.insert(data.indices.end(), {middle, next, current});
            }
        };
        addEnd(-halfLength, 0.02f, glm::vec3(0.0f, -1.0f, 0.0f));
        addEnd(halfLength, 0.98f, glm::vec3(0.0f, 1.0f, 0.0f));

        // The axes of the texture for normal maps (see GenerateTangents). A mesh made here always has triangles.
        [[maybe_unused]] const bool areTangentsGenerated = GenerateTangents(data);
        assert(areTangentsGenerated);

        return data;
    }
}
