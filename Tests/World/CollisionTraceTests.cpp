#include "World/CollisionTrace.h"
#include "World/MapParser.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Abomination::World
{
    namespace
    {
        constexpr double Tolerance = 1e-9;

        // A cube of 2 meters: in the game x from 0 to 2, y (up) from 0 to 2, z from -2 to 0.
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

        // A ramp: in the game x from 0 to 2, y from 0 to 2, z from -2 to 0, with the slope x + y = 2 facing up and to +X.
        // Its sharp edge runs along z at x = 2, y = 0. Faces: bottom, back (x = 0), slope, and the two ends; then the two
        // bevel planes (+X at x = 2 and +Y at y = 2).
        constexpr std::string_view RampMap = R"({
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

        // A cube of 1 meter: half of it in every direction.
        constexpr glm::dvec3 HalfExtents{0.5, 0.5, 0.5};

        std::vector<CollisionBrush> BuildFromMap(std::string_view mapText)
        {
            const std::expected<MapData, std::string> map = ParseMap(mapText);
            EXPECT_TRUE(map.has_value()) << (map.has_value() ? "" : map.error());

            return BuildCollisionBrushes(map.value().entities.at(0));
        }

        // The same brush moved by offset: every point p of a plane becomes p + offset, so dot(normal, p) grows by
        // dot(normal, offset).
        CollisionBrush MoveBrush(CollisionBrush brush, const glm::dvec3& offset)
        {
            for (Core::Plane& plane : brush.planes)
                plane.distance += glm::dot(plane.normal, offset);
            brush.bounds.minimum += offset;
            brush.bounds.maximum += offset;

            return brush;
        }

        void ExpectNear(const glm::dvec3& actual, const glm::dvec3& expected, double tolerance = Tolerance)
        {
            EXPECT_NEAR(actual.x, expected.x, tolerance);
            EXPECT_NEAR(actual.y, expected.y, tolerance);
            EXPECT_NEAR(actual.z, expected.z, tolerance);
        }
    }

    TEST(CollisionTrace, EmptyWorldLetsBoxMoveAllTheWay)
    {
        const TraceResult result = TraceBox({}, {0.0, 0.0, 0.0}, {3.0, 1.0, -2.0}, HalfExtents);

        EXPECT_EQ(result.fraction, 1.0);
        ExpectNear(result.endPosition, {3.0, 1.0, -2.0});
        EXPECT_EQ(result.hitNormal, glm::dvec3(0.0));
        EXPECT_FALSE(result.startsInSolid);
    }

    TEST(CollisionTrace, BoxStopsInFrontOfWall)
    {
        // The box moves along +X from x = -3 to x = 1 into the cube, whose wall is at x = 0. Its center stops half the
        // box (0.5) before the wall, and SurfaceEpsilon more.
        const std::vector<CollisionBrush> brushes = BuildFromMap(CubeMap);

        const TraceResult result = TraceBox(brushes, {-3.0, 1.0, -1.0}, {1.0, 1.0, -1.0}, HalfExtents);

        EXPECT_LT(result.fraction, 1.0);
        ExpectNear(result.endPosition, {-0.5 - SurfaceEpsilon, 1.0, -1.0});
        ExpectNear(result.hitNormal, {-1.0, 0.0, 0.0});
        EXPECT_FALSE(result.startsInSolid);
    }

    TEST(CollisionTrace, BoxPassingBesideBrushIsNotStopped)
    {
        // The same move 1 meter further along +Z: the box (z from 0.5 to 1.5) passes the cube (z from -2 to 0).
        const std::vector<CollisionBrush> brushes = BuildFromMap(CubeMap);

        const TraceResult result = TraceBox(brushes, {-3.0, 1.0, 1.0}, {3.0, 1.0, 1.0}, HalfExtents);

        EXPECT_EQ(result.fraction, 1.0);
        EXPECT_FALSE(result.startsInSolid);
    }

    TEST(CollisionTrace, BoxStoppedAtWallCanMoveAlongIt)
    {
        // After a hit the box is SurfaceEpsilon in front of the wall, not in it, so moving along the wall is free. Without
        // the gap it would touch the wall exactly and could count as inside the brush.
        const std::vector<CollisionBrush> brushes = BuildFromMap(CubeMap);
        const TraceResult hit = TraceBox(brushes, {-3.0, 1.0, -1.0}, {1.0, 1.0, -1.0}, HalfExtents);

        const glm::dvec3 alongWall = hit.endPosition + glm::dvec3(0.0, 0.0, -0.5);
        const TraceResult slide = TraceBox(brushes, hit.endPosition, alongWall, HalfExtents);

        EXPECT_EQ(slide.fraction, 1.0);
        EXPECT_FALSE(slide.startsInSolid);
    }

    TEST(CollisionTrace, BoxInsideBrushIsStuck)
    {
        const std::vector<CollisionBrush> brushes = BuildFromMap(CubeMap);

        const TraceResult result = TraceBox(brushes, {1.0, 1.0, -1.0}, {1.2, 1.0, -1.0}, HalfExtents);

        EXPECT_TRUE(result.startsInSolid);
        EXPECT_TRUE(result.isStuck);
        EXPECT_EQ(result.fraction, 0.0);
    }

    TEST(CollisionTrace, BoxStartingInsideIsNotStuckIfItsWayLeavesTheBrush)
    {
        const std::vector<CollisionBrush> brushes = BuildFromMap(CubeMap);

        const TraceResult result = TraceBox(brushes, {1.0, 1.0, -1.0}, {5.0, 1.0, -1.0}, HalfExtents);

        EXPECT_TRUE(result.startsInSolid);
        EXPECT_FALSE(result.isStuck);
    }

    TEST(CollisionTrace, EarliestOfSeveralBrushesCounts)
    {
        // Two cubes on the way, the nearer one listed last: the order of the brushes does not matter.
        const CollisionBrush cube = BuildFromMap(CubeMap).at(0);
        const std::vector<CollisionBrush> brushes = {MoveBrush(cube, {4.0, 0.0, 0.0}), cube};

        const TraceResult result = TraceBox(brushes, {-3.0, 1.0, -1.0}, {9.0, 1.0, -1.0}, HalfExtents);

        ExpectNear(result.endPosition, {-0.5 - SurfaceEpsilon, 1.0, -1.0});
    }

    TEST(CollisionTrace, BoxFallingOnSlopeGetsSlopeNormal)
    {
        // Falling straight down onto the ramp: the surface hit is the slope, whose normal points up and to +X at 45°.
        const std::vector<CollisionBrush> brushes = BuildFromMap(RampMap);

        const TraceResult result = TraceBox(brushes, {1.0, 4.0, -1.0}, {1.0, -1.0, -1.0}, HalfExtents);

        EXPECT_LT(result.fraction, 1.0);
        const double component = 1.0 / std::sqrt(2.0);
        ExpectNear(result.hitNormal, {component, component, 0.0});
    }

    TEST(CollisionTrace, BevelsRemoveInvisibleWallAtSharpEdge)
    {
        // The box flies down and to the right, past the sharp edge of the ramp (x = 2, y = 0), and never touches it: where
        // its center crosses the moved slope (at (2.64, 0.36)), the left side of the box is at x = 2.14, beyond the edge.
        // Without bevels the moved bottom and slope planes meet far beyond the edge (the "spike"), and the box would hit
        // an invisible wall there. The moved +X bevel (x = 2.5) cuts the spike off: the line leaves it before it enters
        // the moved slope, so there is no hit.
        const CollisionBrush ramp = BuildFromMap(RampMap).at(0);
        const glm::dvec3 start{1.2, 2.6, -1.0};
        const glm::dvec3 end{3.0, -0.2, -1.0};

        const TraceResult withBevels = TraceBox(std::span(&ramp, 1), start, end, HalfExtents);
        EXPECT_EQ(withBevels.fraction, 1.0);

        CollisionBrush withoutBevels = ramp;
        withoutBevels.planes.resize(5); // only the 5 faces, the bevels come after them
        const TraceResult spike = TraceBox(std::span(&withoutBevels, 1), start, end, HalfExtents);
        EXPECT_LT(spike.fraction, 1.0);
    }
}
