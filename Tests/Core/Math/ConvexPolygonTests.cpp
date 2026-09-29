#include "Core/Math/ConvexPolygon.h"

#include <glm/vec3.hpp>
#include <gtest/gtest.h>

namespace Abomination::Core
{
    namespace
    {
        // A square of side 2 in the plane z = 0, from (-1, -1) to (1, 1), counter-clockwise seen from above.
        const ConvexPolygon Square{{-1.0, -1.0, 0.0}, {1.0, -1.0, 0.0}, {1.0, 1.0, 0.0}, {-1.0, 1.0, 0.0}};

        // The plane x = 0 facing +X: its front is the right half of the square.
        const Plane VerticalPlaneThroughCenter{.normal = {1.0, 0.0, 0.0}, .distanceFromOrigin = 0.0};
    }

    TEST(ConvexPolygon, KeepsPartBehindPlane)
    {
        const ConvexPolygon result = ClipPolygon(Square, VerticalPlaneThroughCenter);

        // The left half: two old corners and two new ones where the plane crosses the bottom and top edges,
        // in the same (counter-clockwise) order.
        const ConvexPolygon expected{{-1.0, -1.0, 0.0}, {0.0, -1.0, 0.0}, {0.0, 1.0, 0.0}, {-1.0, 1.0, 0.0}};
        EXPECT_EQ(result, expected);
    }

    TEST(ConvexPolygon, CrossingPointFollowsDistances)
    {
        // The plane x = 0.5: distances of the bottom edge ends are -1.5 and 0.5, so the crossing is at 3/4 of the edge.
        const Plane plane{.normal = {1.0, 0.0, 0.0}, .distanceFromOrigin = 0.5};

        const ConvexPolygon result = ClipPolygon(Square, plane);

        ASSERT_EQ(result.size(), 4u);
        EXPECT_DOUBLE_EQ(result[1].x, 0.5);
        EXPECT_DOUBLE_EQ(result[1].y, -1.0);
    }

    TEST(ConvexPolygon, PolygonBehindPlaneStaysWhole)
    {
        const Plane planeFarRight{.normal = {1.0, 0.0, 0.0}, .distanceFromOrigin = 5.0};

        EXPECT_EQ(ClipPolygon(Square, planeFarRight), Square);
    }

    TEST(ConvexPolygon, PolygonInFrontOfPlaneDisappears)
    {
        const Plane planeFarLeft{.normal = {1.0, 0.0, 0.0}, .distanceFromOrigin = -5.0};

        EXPECT_TRUE(ClipPolygon(Square, planeFarLeft).empty());
    }

    TEST(ConvexPolygon, CornerOnPlaneIsKeptOnce)
    {
        // The diagonal plane through the corners (1, -1) and (-1, 1), facing towards (1, 1): the corner (1, 1) is in
        // front, the two corners on the plane stay without duplicates, and a triangle is left.
        const Plane diagonal{.normal = {0.70710678118654752, 0.70710678118654752, 0.0}, .distanceFromOrigin = 0.0};

        const ConvexPolygon result = ClipPolygon(Square, diagonal);

        const ConvexPolygon expected{{-1.0, -1.0, 0.0}, {1.0, -1.0, 0.0}, {-1.0, 1.0, 0.0}};
        EXPECT_EQ(result, expected);
    }
}
