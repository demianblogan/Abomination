#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>

namespace Abomination::Renderer
{
    // One vertex as it lies in the vertex buffer: position, texture coordinates, normal and tangent.
    //   bytes:  0              12         20             32                  48
    //           | x | y | z | u | v | nx | ny | nz | tx | ty | tz | tw |
    struct MeshVertex
    {
        glm::vec3 position{0.0f};
        glm::vec2 texCoord{0.0f};

        // The direction the surface faces at this vertex (length 1). Shaders use it to shade surfaces by how they are
        // turned: the level has no lighting yet (0.5), and without shading its walls and floor would look the same.
        glm::vec3 normal{0.0f, 1.0f, 0.0f};

        // The axes of the texture on the surface, for normal maps (see Documentation/ARCHITECTURE.md, section 6, materials):
        // xyz is the tangent, the direction in which the U texture coordinate grows (length 1, along the surface); w is +1
        // or -1, and cross(normal, tangent.xyz) * w is the bitangent, the direction in which V grows. The convention of
        // glTF. w is -1 where the texture is mirrored.
        glm::vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
    };

    // How one vertex of a skinned mesh follows the bones of its skeleton: up to four joints (indices into
    // SkeletonData::joints) and how much each of them pulls it (the weights add up to 1). A vertex of an elbow is pulled
    // by the upper arm and the forearm, half each, so the skin bends smoothly instead of breaking at the joint.
    struct VertexSkin
    {
        glm::uvec4 joints{0};
        glm::vec4 weights{1.0f, 0.0f, 0.0f, 0.0f};
    };

    // The geometry of a mesh in ordinary memory, before it is uploaded to the GPU: vertices and the indices that make
    // triangles of them (every 3 indices are one triangle, counter-clockwise when looked at from its front side).
    // Built by code (MeshPrimitives, the level) or read from model files (ModelData). Needs no OpenGL, so it can be tested.
    struct MeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<std::uint32_t> indices;

        // The skin of every vertex, in the same order, for a mesh bent by a skeleton; empty for a rigid mesh.
        std::vector<VertexSkin> skin;
    };
}
