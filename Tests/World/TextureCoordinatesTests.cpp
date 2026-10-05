#include "World/TextureCoordinates.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>

namespace Abomination::World
{
    namespace
    {
        constexpr float Tolerance = 1e-6f;
        constexpr glm::ivec2 TextureSize{64, 64};

        // A wall facing -Y, as TrenchBroom writes it: U along world +X, V down (world -Z), no offset, scale 1.
        MapFace CreateWallFace()
        {
            return MapFace{
                .textureUAxis = {1.0, 0.0, 0.0},
                .textureVAxis = {0.0, 0.0, -1.0},
            };
        }

        void ExpectNear(glm::vec2 actual, glm::vec2 expected)
        {
            EXPECT_NEAR(actual.x, expected.x, Tolerance);
            EXPECT_NEAR(actual.y, expected.y, Tolerance);
        }
    }

    TEST(TextureCoordinates, OneTexturePerTextureSizeInUnits)
    {
        const MapFace face = CreateWallFace();

        // At scale 1 one texel is one unit: 64 units along U is the texture once, 96 units one and a half times.
        ExpectNear(CalculateTextureCoordinates(face, {0.0, 0.0, 0.0}, TextureSize), {0.0f, 0.0f});
        ExpectNear(CalculateTextureCoordinates(face, {64.0, 0.0, 0.0}, TextureSize), {1.0f, 0.0f});
        ExpectNear(CalculateTextureCoordinates(face, {96.0, 0.0, 0.0}, TextureSize), {1.5f, 0.0f});
    }

    TEST(TextureCoordinates, UpInTheWorldIsUpInTheTexture)
    {
        const MapFace face = CreateWallFace();

        // The V axis points down, so 32 units higher is 32 texels less along V: -0.5 in the map's convention, +0.5 in
        // OpenGL's, where V grows up. The top of the image is at the top of the wall.
        ExpectNear(CalculateTextureCoordinates(face, {0.0, 0.0, 32.0}, TextureSize), {0.0f, 0.5f});
    }

    TEST(TextureCoordinates, OffsetMovesTextureInTexels)
    {
        MapFace face = CreateWallFace();
        face.textureOffsetU = 16.0;
        face.textureOffsetV = 32.0;

        ExpectNear(CalculateTextureCoordinates(face, {0.0, 0.0, 0.0}, TextureSize), {0.25f, -0.5f});
    }

    TEST(TextureCoordinates, ScaleIsUnitsPerTexel)
    {
        MapFace face = CreateWallFace();
        face.textureScaleU = 2.0; // the texture is stretched to twice its size: 128 units per repeat
        face.textureScaleV = 0.5;

        ExpectNear(CalculateTextureCoordinates(face, {64.0, 0.0, -16.0}, TextureSize), {0.5f, -0.5f});
    }

    TEST(TextureCoordinates, RotatedAxesFollowTheFace)
    {
        // A texture turned by 90 degrees on the floor: U runs along world +Y instead of +X.
        const MapFace face{
            .textureUAxis = {0.0, 1.0, 0.0},
            .textureVAxis = {1.0, 0.0, 0.0},
        };

        ExpectNear(CalculateTextureCoordinates(face, {0.0, 32.0, 0.0}, TextureSize), {0.5f, 0.0f});
        ExpectNear(CalculateTextureCoordinates(face, {32.0, 0.0, 0.0}, TextureSize), {0.0f, -0.5f});
    }

    TEST(TextureCoordinates, RectangularTextureUsesItsOwnWidthAndHeight)
    {
        const MapFace face = CreateWallFace();

        ExpectNear(CalculateTextureCoordinates(face, {64.0, 0.0, -64.0}, {128, 32}), {0.5f, -2.0f});
    }

    TEST(TextureCoordinates, ZeroScaleCountsAsOne)
    {
        MapFace face = CreateWallFace();
        face.textureScaleU = 0.0;

        ExpectNear(CalculateTextureCoordinates(face, {32.0, 0.0, 0.0}, TextureSize), {0.5f, 0.0f});
    }

    TEST(TextureCoordinates, TangentOfWallPointsAlongUAndUp)
    {
        // The wall faces -Y of the map: +Z in the game. U grows along +X; V (OpenGL) grows up, along +Y of the game.
        const glm::vec4 tangent = CalculateTangent(CreateWallFace(), {0.0f, 0.0f, 1.0f});

        EXPECT_NEAR(tangent.x, 1.0f, Tolerance);
        EXPECT_NEAR(tangent.y, 0.0f, Tolerance);
        EXPECT_NEAR(tangent.z, 0.0f, Tolerance);

        // The bitangent cross(normal, tangent) * w = (0, 1, 0): up the wall, where V grows.
        EXPECT_FLOAT_EQ(tangent.w, 1.0f);
    }

    TEST(TextureCoordinates, MirroredTextureTurnsTangentAndSign)
    {
        // A negative scale turns the texture around along U: U grows towards -X, and V still grows up, so the axes of the
        // texture are mirrored and w becomes -1.
        MapFace face = CreateWallFace();
        face.textureScaleU = -1.0;

        const glm::vec4 tangent = CalculateTangent(face, {0.0f, 0.0f, 1.0f});

        EXPECT_NEAR(tangent.x, -1.0f, Tolerance);
        EXPECT_FLOAT_EQ(tangent.w, -1.0f);
    }

    TEST(TextureCoordinates, TangentIsLaidOnTheFace)
    {
        // A U axis that leaves the face (a texture projected from another side): the tangent is still along the wall.
        MapFace face = CreateWallFace();
        face.textureUAxis = {1.0, -0.5, 0.0};

        const glm::vec4 tangent = CalculateTangent(face, {0.0f, 0.0f, 1.0f});

        EXPECT_NEAR(tangent.x, 1.0f, Tolerance);
        EXPECT_NEAR(tangent.z, 0.0f, Tolerance);
    }
}
