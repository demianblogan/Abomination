#include "Core/Math/Plane.h"

#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <optional>

namespace Abomination::Core
{
    namespace
    {
        constexpr double Tolerance = 1e-9;
    }

    TEST(Plane, BuildsNormalAndDistanceFromThreePoints)
    {
        // The inner face of a wall from the test map: all points have x = 48, and the normal points to -X
        // (out of the wall, into the room).
        const std::optional<Plane> plane =
            CreatePlaneFromPoints({48.0, 64.0, 240.0}, {48.0, -448.0, 240.0}, {48.0, 64.0, -16.0});

        ASSERT_TRUE(plane.has_value());
        EXPECT_NEAR(plane->normal.x, -1.0, Tolerance);
        EXPECT_NEAR(plane->normal.y, 0.0, Tolerance);
        EXPECT_NEAR(plane->normal.z, 0.0, Tolerance);

        // dot((-1, 0, 0), (48, ...)) = -48.
        EXPECT_NEAR(plane->distance, -48.0, Tolerance);
    }

    TEST(Plane, SwappingTwoPointsTurnsNormalAround)
    {
        const std::optional<Plane> plane =
            CreatePlaneFromPoints({48.0, -448.0, 240.0}, {48.0, 64.0, 240.0}, {48.0, 64.0, -16.0});

        ASSERT_TRUE(plane.has_value());
        EXPECT_NEAR(plane->normal.x, 1.0, Tolerance);
    }

    TEST(Plane, PointsOnOneLineGiveNoPlane)
    {
        EXPECT_FALSE(CreatePlaneFromPoints({0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}, {2.0, 2.0, 2.0}).has_value());
        EXPECT_FALSE(CreatePlaneFromPoints({5.0, 0.0, 0.0}, {5.0, 0.0, 0.0}, {0.0, 3.0, 0.0}).has_value());
    }

    TEST(Plane, SignedDistanceTellsSideAndDistance)
    {
        // The plane z = 10, facing up.
        const Plane plane{.normal = {0.0, 0.0, 1.0}, .distance = 10.0};

        EXPECT_DOUBLE_EQ(CalculateSignedDistance(plane, {3.0, 4.0, 25.0}), 15.0);  // 15 units in front
        EXPECT_DOUBLE_EQ(CalculateSignedDistance(plane, {0.0, 0.0, 4.0}), -6.0);   // 6 units behind
        EXPECT_DOUBLE_EQ(CalculateSignedDistance(plane, {-7.0, 2.0, 10.0}), 0.0);  // on the plane
    }
}
