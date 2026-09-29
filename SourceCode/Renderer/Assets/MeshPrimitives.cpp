#include "Renderer/Assets/MeshPrimitives.h"

#include <glm/vec3.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

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

        return data;
    }
}
