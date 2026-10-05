#include "Renderer/Assets/MeshTangents.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

namespace Abomination::Renderer
{
    namespace
    {
        constexpr float Tolerance = 1e-5f;

        // A square of 1 x 1 m in the XY plane facing +Z, two triangles, with the texture laid on it once: u along +X,
        // v along +Y. mirrorU lays it the other way round along X.
        MeshData CreateSquare(bool mirrorU)
        {
            const float left = mirrorU ? 1.0f : 0.0f;
            const float right = mirrorU ? 0.0f : 1.0f;
            const glm::vec3 normal(0.0f, 0.0f, 1.0f);

            MeshData mesh;
            mesh.vertices = {
                {.position = {0.0f, 0.0f, 0.0f}, .texCoord = {left, 0.0f}, .normal = normal},
                {.position = {1.0f, 0.0f, 0.0f}, .texCoord = {right, 0.0f}, .normal = normal},
                {.position = {1.0f, 1.0f, 0.0f}, .texCoord = {right, 1.0f}, .normal = normal},
                {.position = {0.0f, 1.0f, 0.0f}, .texCoord = {left, 1.0f}, .normal = normal},
            };
            mesh.indices = {0, 1, 2, 2, 3, 0};
            return mesh;
        }
    }

    TEST(MeshTangents, TangentPointsWhereUGrows)
    {
        MeshData square = CreateSquare(false);

        ASSERT_TRUE(GenerateTangents(square));

        for (const MeshVertex& vertex : square.vertices)
        {
            EXPECT_NEAR(vertex.tangent.x, 1.0f, Tolerance);
            EXPECT_NEAR(vertex.tangent.y, 0.0f, Tolerance);
            EXPECT_NEAR(vertex.tangent.z, 0.0f, Tolerance);

            // cross(+Z, +X) = +Y, where v grows: no mirroring.
            EXPECT_FLOAT_EQ(vertex.tangent.w, 1.0f);
        }
    }

    TEST(MeshTangents, MirroredTextureGivesNegativeSign)
    {
        MeshData square = CreateSquare(true);

        ASSERT_TRUE(GenerateTangents(square));

        for (const MeshVertex& vertex : square.vertices)
        {
            EXPECT_NEAR(vertex.tangent.x, -1.0f, Tolerance);
            EXPECT_FLOAT_EQ(vertex.tangent.w, -1.0f);
        }
    }
}
