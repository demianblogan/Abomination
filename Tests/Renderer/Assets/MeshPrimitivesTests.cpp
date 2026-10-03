#include "Renderer/Assets/MeshPrimitives.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace Abomination::Renderer
{
    TEST(MeshPrimitives, CubeHasFourVerticesAndTwoTrianglesPerFace)
    {
        const MeshData cube = CreateCubeMeshData();

        EXPECT_EQ(cube.vertices.size(), 6u * 4u);
        EXPECT_EQ(cube.indices.size(), 6u * 2u * 3u);
        for (const std::uint32_t index : cube.indices)
            EXPECT_LT(index, cube.vertices.size());
    }

    TEST(MeshPrimitives, CubeIsCenteredWithSizeOne)
    {
        const MeshData cube = CreateCubeMeshData();

        for (const MeshVertex& vertex : cube.vertices)
            for (int axis = 0; axis < 3; ++axis)
                EXPECT_FLOAT_EQ(std::abs(vertex.position[axis]), 0.5f);
    }

    TEST(MeshPrimitives, CubeNormalsPointOutOfTheirFaces)
    {
        // On a cube centered at the origin, the normal of a face points the same way as the position of its vertices
        // along one axis: the vertices of the top face have y = 0.5 and the normal (0, 1, 0).
        const MeshData cube = CreateCubeMeshData();

        for (const MeshVertex& vertex : cube.vertices)
        {
            EXPECT_FLOAT_EQ(glm::length(vertex.normal), 1.0f);
            EXPECT_FLOAT_EQ(glm::dot(vertex.normal, vertex.position), 0.5f);
        }
    }

    TEST(MeshPrimitives, CubeTrianglesFaceOutwards)
    {
        // Face culling keeps only triangles whose vertices go counter-clockwise on the screen. For a triangle A, B, C
        // that is the case when the cross product (B - A) x (C - A) points towards the viewer, so for a cube seen from
        // outside it must point away from the center: the same direction as the triangle's center itself.
        const MeshData cube = CreateCubeMeshData();

        for (std::size_t first = 0; first < cube.indices.size(); first += 3)
        {
            const glm::vec3 a = cube.vertices[cube.indices[first]].position;
            const glm::vec3 b = cube.vertices[cube.indices[first + 1]].position;
            const glm::vec3 c = cube.vertices[cube.indices[first + 2]].position;

            const glm::vec3 normal = glm::cross(b - a, c - a);
            const glm::vec3 triangleCenter = (a + b + c) / 3.0f;

            EXPECT_GT(glm::dot(normal, triangleCenter), 0.0f) << "triangle starting at index " << first;
        }
    }

    TEST(MeshPrimitives, CylinderHasSidesAndClosedEnds)
    {
        // 8 sides: 9 columns of 2 vertices (the seam twice), and per end a middle and 8 corners.
        const MeshData cylinder = CreateCylinderMeshData(8, 0.5f, 2.0f);

        EXPECT_EQ(cylinder.vertices.size(), 9u * 2u + 2u * 9u);
        EXPECT_EQ(cylinder.indices.size(), (8u * 2u + 2u * 8u) * 3u);
        for (const std::uint32_t index : cylinder.indices)
            EXPECT_LT(index, cylinder.vertices.size());
        for (const MeshVertex& vertex : cylinder.vertices)
        {
            EXPECT_LE(std::abs(vertex.position.y), 1.0f + 1e-6f);
            EXPECT_LE(glm::length(glm::vec3(vertex.position.x, 0.0f, vertex.position.z)), 0.5f + 1e-6f);
        }
    }

    TEST(MeshPrimitives, CylinderTrianglesFaceOutwards)
    {
        // Like the cube: the cross product of every triangle points away from the middle of the cylinder.
        const MeshData cylinder = CreateCylinderMeshData(8, 0.5f, 2.0f);

        for (std::size_t first = 0; first < cylinder.indices.size(); first += 3)
        {
            const glm::vec3 a = cylinder.vertices[cylinder.indices[first]].position;
            const glm::vec3 b = cylinder.vertices[cylinder.indices[first + 1]].position;
            const glm::vec3 c = cylinder.vertices[cylinder.indices[first + 2]].position;

            const glm::vec3 normal = glm::cross(b - a, c - a);
            const glm::vec3 triangleCenter = (a + b + c) / 3.0f;
            EXPECT_GT(glm::dot(normal, triangleCenter), 0.0f) << "triangle starting at index " << first;
        }
    }
}
