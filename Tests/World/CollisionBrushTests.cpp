#include "World/CollisionBrush.h"
#include "World/MapParser.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace Abomination::World
{
    namespace
    {
        constexpr double Tolerance = 1e-9;

        // A cube of 64 units (2 meters) from the map origin: x and y from 0 to 64, z from 0 to 64.
        // In the game: x from 0 to 2, y (up) from 0 to 2, z from -2 to 0.
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

        // A wedge (a ramp seen from the side): the triangle (0, 0, 0) - (64, 0, 0) - (0, 0, 64) stretched along map Y
        // from 0 to 64. Faces: bottom, back (x = 0), slope (x + z = 64), front (y = 0) and rear (y = 64). It has no top
        // face (map +Z, game +Y) and no face along map +X (game +X): the slope replaces both.
        constexpr std::string_view WedgeMap = R"({
"classname" "worldspawn"
{
( 0 64 0 ) ( 0 0 0 ) ( 64 0 0 ) Crate [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 0 0 64 ) ( 0 0 0 ) ( 0 64 0 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 64 64 0 ) ( 64 0 0 ) ( 0 0 64 ) Crate [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 64 0 0 ) ( 0 0 0 ) ( 0 0 64 ) Crate [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 0 64 64 ) ( 0 64 0 ) ( 64 64 0 ) Crate [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
}
}
)";

        std::vector<CollisionBrush> BuildFromMap(std::string_view mapText)
        {
            const std::expected<MapData, std::string> map = ParseMap(mapText);
            EXPECT_TRUE(map.has_value()) << (map.has_value() ? "" : map.error());

            return BuildCollisionBrushes(map.value().entities.at(0));
        }

        void ExpectNear(const glm::dvec3& actual, const glm::dvec3& expected)
        {
            EXPECT_NEAR(actual.x, expected.x, Tolerance);
            EXPECT_NEAR(actual.y, expected.y, Tolerance);
            EXPECT_NEAR(actual.z, expected.z, Tolerance);
        }

        bool HasPlane(const CollisionBrush& brush, const glm::dvec3& normal, double distanceFromOrigin)
        {
            for (const Core::Plane& plane : brush.planes)
                if (glm::length(plane.normal - normal) < Tolerance &&
                    std::abs(plane.distanceFromOrigin - distanceFromOrigin) < Tolerance)
                    return true;

            return false;
        }
    }

    TEST(CollisionBrush, CubeHasSixPlanesAndNoBevels)
    {
        const std::vector<CollisionBrush> brushes = BuildFromMap(CubeMap);

        ASSERT_EQ(brushes.size(), 1u);
        // All six sides of its box are faces already, so nothing is added.
        EXPECT_EQ(brushes[0].planes.size(), 6u);
    }

    TEST(CollisionBrush, PlanesAndBoundsAreInGameMetersAndAxes)
    {
        const std::vector<CollisionBrush> brushes = BuildFromMap(CubeMap);
        ASSERT_EQ(brushes.size(), 1u);
        const CollisionBrush& cube = brushes[0];

        ExpectNear(cube.bounds.minimum, {0.0, 0.0, -2.0});
        ExpectNear(cube.bounds.maximum, {2.0, 2.0, 0.0});

        // The top face (map z = 64, normal map +Z) is the plane y = 2 with the normal game +Y.
        EXPECT_TRUE(HasPlane(cube, {0.0, 1.0, 0.0}, 2.0));
        // The face at map y = 64 (normal map +Y) is the plane z = -2 with the normal game -Z: dot((0, 0, -1), p) = 2.
        EXPECT_TRUE(HasPlane(cube, {0.0, 0.0, -1.0}, 2.0));
    }

    TEST(CollisionBrush, WedgeGetsBevelsForMissingSides)
    {
        const std::vector<CollisionBrush> brushes = BuildFromMap(WedgeMap);
        ASSERT_EQ(brushes.size(), 1u);
        const CollisionBrush& wedge = brushes[0];

        // 5 faces + 2 bevels: the top of its box (game +Y at y = 2) and the side at game x = 2.
        EXPECT_EQ(wedge.planes.size(), 7u);
        EXPECT_TRUE(HasPlane(wedge, {1.0, 0.0, 0.0}, 2.0));
        EXPECT_TRUE(HasPlane(wedge, {0.0, 1.0, 0.0}, 2.0));
    }

    TEST(CollisionBrush, BevelsCutNothingOffTheBrush)
    {
        // Every corner of the brush is behind or on every plane, the bevels included: the bevels do not make the brush
        // smaller, they only matter once the planes are moved out for a box.
        const std::vector<CollisionBrush> brushes = BuildFromMap(WedgeMap);
        ASSERT_EQ(brushes.size(), 1u);
        const CollisionBrush& wedge = brushes[0];

        const std::vector<glm::dvec3> corners = {
            {0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {0.0, 2.0, 0.0}, {0.0, 0.0, -2.0}, {2.0, 0.0, -2.0}, {0.0, 2.0, -2.0},
        };
        for (const glm::dvec3& corner : corners)
            for (const Core::Plane& plane : wedge.planes)
                EXPECT_LE(Core::CalculateSignedDistance(plane, corner), Tolerance);
    }

    TEST(CollisionBrush, FaceWithoutPlaneIsLeftOut)
    {
        const std::expected<MapData, std::string> map = ParseMap(CubeMap);
        ASSERT_TRUE(map.has_value());
        MapEntity world = map.value().entities.at(0);
        world.brushes[0].faces.push_back(MapFace{.points = {glm::dvec3(0.0), glm::dvec3(1.0), glm::dvec3(2.0)}});

        const std::vector<CollisionBrush> brushes = BuildCollisionBrushes(world);

        ASSERT_EQ(brushes.size(), 1u);
        EXPECT_EQ(brushes[0].planes.size(), 6u);
    }
}
