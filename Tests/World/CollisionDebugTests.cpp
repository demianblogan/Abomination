#include "World/CollisionDebug.h"
#include "World/MapParser.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

namespace Abomination::World
{
    namespace
    {
        // A cube of 2 meters: in the game x from 0 to 2, y from 0 to 2, z from -2 to 0.
        constexpr std::string_view CubeMap = R"({
"classname" "worldspawn"
{
( 0 0 64 ) ( 0 0 0 ) ( 0 64 0 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 64 64 0 ) ( 64 0 0 ) ( 64 0 64 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 64 0 0 ) ( 0 0 0 ) ( 0 0 64 ) Crate [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 0 64 64 ) ( 0 64 0 ) ( 64 64 0 ) Crate [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 0 64 0 ) ( 0 0 0 ) ( 64 0 0 ) Crate [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 64 0 64 ) ( 0 0 64 ) ( 0 64 64 ) Crate [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
}
}
)";

        std::vector<CollisionBrush> BuildCube()
        {
            const std::expected<MapData, std::string> map = ParseMap(CubeMap);
            EXPECT_TRUE(map.has_value());

            return BuildCollisionBrushes(map.value().entities.at(0));
        }

        // A camera 5 meters in front of the cube (along +Z), looking at it: along -Z, which is where a camera without
        // rotation looks.
        Core::Transform CreateCameraFacingCube()
        {
            return Core::Transform{.position = {1.0f, 1.0f, 5.0f}};
        }
    }

    TEST(CollisionDebug, NothingEnabledAddsNoLines)
    {
        Renderer::DebugLines lines;

        const CameraCast cast = UpdateCollisionDebug(BuildCube(), {}, CreateCameraFacingCube(), lines);

        EXPECT_EQ(lines.GetLineCount(), 0u);
        EXPECT_FALSE(cast.isValid);
    }

    TEST(CollisionDebug, BrushBoundsAreOneBoxPerBrush)
    {
        Renderer::DebugLines lines;

        const CollisionDebugSettings settings{.areColliderBoundsVisible = true};
        UpdateCollisionDebug(BuildCube(), settings, CreateCameraFacingCube(), lines);

        EXPECT_EQ(lines.GetLineCount(), 12u);
    }

    TEST(CollisionDebug, CameraCastStopsAtCubeAndShowsNormal)
    {
        Renderer::DebugLines lines;
        const CollisionDebugSettings settings{.isCameraCastEnabled = true, .cameraCastShape = CastShape::PlayerBox};

        const CameraCast cast = UpdateCollisionDebug(BuildCube(), settings, CreateCameraFacingCube(), lines);

        // The front of the cube is at z = 0, the player box reaches 0.5 m forward: it stops 4.5 m ahead.
        ASSERT_TRUE(cast.isValid);
        EXPECT_NEAR(cast.distance, 4.5, 0.01);
        EXPECT_EQ(cast.result.hitNormal, glm::dvec3(0.0, 0.0, 1.0));
        EXPECT_EQ(lines.GetLineCount(), 12u + 5u); // the box and the normal arrow
    }

    TEST(CollisionDebug, CameraCastIntoEmptySpaceFliesAllTheWay)
    {
        // The camera turned around (180 degrees around Y) looks away from the cube.
        Core::Transform camera = CreateCameraFacingCube();
        camera.rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        Renderer::DebugLines lines;

        const CameraCast cast = UpdateCollisionDebug(BuildCube(), {.isCameraCastEnabled = true}, camera, lines);

        EXPECT_EQ(cast.result.fraction, 1.0);
        EXPECT_NEAR(cast.distance, CameraCastLength, 1e-9);
        EXPECT_EQ(lines.GetLineCount(), 12u); // only the box, no normal
    }

    TEST(CollisionDebug, CastShapesHaveTheirSizes)
    {
        EXPECT_EQ(GetHalfExtents(CastShape::Ray), glm::dvec3(0.0));
        EXPECT_EQ(GetHalfExtents(CastShape::PlayerBox), PlayerHalfExtents);
    }
}
