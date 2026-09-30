#include "Core/Math/BoundingBox.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <optional>
#include <vector>

namespace Abomination::Core
{
    TEST(BoundingBox, NoPointsGiveBoxAtOrigin)
    {
        const BoundingBox box = CalculateBoundingBox({});

        EXPECT_EQ(box.minimum, glm::dvec3(0.0));
        EXPECT_EQ(box.maximum, glm::dvec3(0.0));
    }

    TEST(BoundingBox, OnePointGivesBoxOfZeroSizeAtThatPoint)
    {
        const std::vector<glm::dvec3> points = {{1.0, 2.0, 3.0}};

        const BoundingBox box = CalculateBoundingBox(points);

        EXPECT_EQ(box.minimum, glm::dvec3(1.0, 2.0, 3.0));
        EXPECT_EQ(box.maximum, glm::dvec3(1.0, 2.0, 3.0));
    }

    TEST(BoundingBox, CornersOfCubeGiveThatCube)
    {
        // The 8 corners of a cube from (0, 0, 0) to (2, 2, 2), in no particular order.
        const std::vector<glm::dvec3> points = {
            {2.0, 0.0, 2.0}, {0.0, 0.0, 0.0}, {2.0, 2.0, 2.0}, {0.0, 2.0, 0.0},
            {2.0, 0.0, 0.0}, {0.0, 0.0, 2.0}, {2.0, 2.0, 0.0}, {0.0, 2.0, 2.0},
        };

        const BoundingBox box = CalculateBoundingBox(points);

        EXPECT_EQ(box.minimum, glm::dvec3(0.0, 0.0, 0.0));
        EXPECT_EQ(box.maximum, glm::dvec3(2.0, 2.0, 2.0));
    }

    TEST(BoundingBox, EveryAxisIsFoundOnItsOwn)
    {
        // No single point is the minimum or the maximum: the smallest x comes from the first point, the smallest y from
        // the second, the smallest z from the third, and so on.
        const std::vector<glm::dvec3> points = {{-5.0, 1.0, 1.0}, {1.0, -6.0, 1.0}, {1.0, 1.0, -7.0}, {3.0, 4.0, 5.0}};

        const BoundingBox box = CalculateBoundingBox(points);

        EXPECT_EQ(box.minimum, glm::dvec3(-5.0, -6.0, -7.0));
        EXPECT_EQ(box.maximum, glm::dvec3(3.0, 4.0, 5.0));
    }

    TEST(BoundingBox, NegativePointsOnlyAreHandled)
    {
        // A box far from the origin, all coordinates negative: starting the search from (0, 0, 0) would get it wrong.
        const std::vector<glm::dvec3> points = {{-10.0, -20.0, -30.0}, {-12.0, -18.0, -31.0}};

        const BoundingBox box = CalculateBoundingBox(points);

        EXPECT_EQ(box.minimum, glm::dvec3(-12.0, -20.0, -31.0));
        EXPECT_EQ(box.maximum, glm::dvec3(-10.0, -18.0, -30.0));
    }

    namespace
    {
        // A cube from (-1, -1, -1) to (1, 1, 1) for the ray tests.
        constexpr BoundingBox UnitCube{.minimum = {-1.0, -1.0, -1.0}, .maximum = {1.0, 1.0, 1.0}};
    }

    TEST(BoundingBox, RayEntersBoxAtNearSide)
    {
        const std::optional<double> distance = IntersectRay(UnitCube, {-5.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 100.0);

        ASSERT_TRUE(distance.has_value());
        EXPECT_DOUBLE_EQ(*distance, 4.0);
    }

    TEST(BoundingBox, RayGoingTheOtherWayMisses)
    {
        EXPECT_FALSE(IntersectRay(UnitCube, {-5.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}, 100.0).has_value());
    }

    TEST(BoundingBox, RayPassingBesideMisses)
    {
        EXPECT_FALSE(IntersectRay(UnitCube, {-5.0, 2.0, 0.0}, {1.0, 0.0, 0.0}, 100.0).has_value());
    }

    TEST(BoundingBox, BoxBeyondMaxDistanceIsMissed)
    {
        EXPECT_FALSE(IntersectRay(UnitCube, {-5.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, 3.0).has_value());
    }

    TEST(BoundingBox, RayStartingInsideHitsAtOnce)
    {
        const std::optional<double> distance = IntersectRay(UnitCube, {0.0, 0.0, 0.0}, {0.0, 0.0, -1.0}, 100.0);

        ASSERT_TRUE(distance.has_value());
        EXPECT_DOUBLE_EQ(*distance, 0.0);
    }

    TEST(BoundingBox, SlantedRayHitsCorner)
    {
        // Diagonally towards the corner (1, 1, 1) from (3, 3, 3): it enters the box right at that corner.
        const glm::dvec3 direction = glm::normalize(glm::dvec3(-1.0, -1.0, -1.0));
        const std::optional<double> distance = IntersectRay(UnitCube, {3.0, 3.0, 3.0}, direction, 100.0);

        ASSERT_TRUE(distance.has_value());
        EXPECT_NEAR(*distance, glm::length(glm::dvec3(2.0, 2.0, 2.0)), 1e-9);
    }
}
