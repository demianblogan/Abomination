#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace Abomination::Renderer
{
    // One vertex as it lies in the vertex buffer: position, texture coordinates and normal.
    //   bytes:  0              12         20             32
    //           | x | y | z | u | v | nx | ny | nz |
    struct MeshVertex
    {
        glm::vec3 position{0.0f};
        glm::vec2 texCoord{0.0f};

        // The direction the surface faces at this vertex (length 1). Shaders use it to shade surfaces by how they are
        // turned: the level has no lighting yet (0.5), and without shading its walls and floor would look the same.
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
    };

    // The geometry of a mesh in ordinary memory, before it is uploaded to the GPU: vertices and the indices that make
    // triangles of them (every 3 indices are one triangle, counter-clockwise when looked at from its front side).
    // Built by code (MeshPrimitives) now, loaded from model files later (0.3). Needs no OpenGL, so it can be tested.
    struct MeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;
    };
}
