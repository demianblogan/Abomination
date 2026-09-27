#include "Physics/CharacterMovement.h"

#include "World/CollisionTrace.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <vector>

namespace Abomination::Physics
{
    namespace
    {
        constexpr float Tolerance = 1e-4f;
        constexpr float TickDuration = 1.0f / 60.0f;
        constexpr glm::dvec3 HalfExtents{0.5, 0.875, 0.5};

        // A box brush with its six sides as planes, straight from its corners: enough for movement tests, which do not
        // need to go through a map.
        World::CollisionBrush CreateBoxBrush(const glm::dvec3& minimum, const glm::dvec3& maximum)
        {
            World::CollisionBrush brush;
            brush.bounds = {.minimum = minimum, .maximum = maximum};
            const std::array<glm::dvec3, 3> axes = {glm::dvec3(1, 0, 0), glm::dvec3(0, 1, 0), glm::dvec3(0, 0, 1)};
            for (const glm::dvec3& axis : axes)
            {
                brush.planes.push_back({.normal = axis, .distance = glm::dot(axis, maximum)});
                brush.planes.push_back({.normal = -axis, .distance = glm::dot(-axis, minimum)});
            }

            return brush;
        }

        // A floor with its top at y = 0, and a wall standing on it at x from 2 to 3.
        std::vector<World::CollisionBrush> CreateFloorAndWall()
        {
            return {
                CreateBoxBrush({-50.0, -1.0, -50.0}, {50.0, 0.0, 50.0}),
                CreateBoxBrush({2.0, 0.0, -50.0}, {3.0, 5.0, 50.0}),
            };
        }
    }

    TEST(CharacterMovement, ClipVelocityRemovesPartIntoSurface)
    {
        // Moving right and down into a floor: only the part along the floor is left.
        EXPECT_EQ(ClipVelocity({3.0f, -4.0f, 0.0f}, {0.0f, 1.0f, 0.0f}), glm::vec3(3.0f, 0.0f, 0.0f));

        // Moving away from the surface is not changed.
        EXPECT_EQ(ClipVelocity({3.0f, 4.0f, 0.0f}, {0.0f, 1.0f, 0.0f}), glm::vec3(3.0f, 4.0f, 0.0f));
    }

    TEST(CharacterMovement, CharacterInAirFallsWithGravity)
    {
        CharacterBody body{.halfExtents = HalfExtents};
        Core::Transform transform{.position = {0.0f, 10.0f, 0.0f}};

        UpdateCharacter(body, transform, CreateFloorAndWall(), PhysicsSettings{.gravity = 25.0f}, TickDuration);

        // One tick of gravity: the speed grows by g * dt downwards.
        EXPECT_NEAR(body.velocity.y, -25.0f * TickDuration, Tolerance);
        EXPECT_LT(transform.position.y, 10.0f);
        EXPECT_FALSE(body.isOnGround);
    }

    TEST(CharacterMovement, FallingCharacterLandsOnFloor)
    {
        const std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        CharacterBody body{.halfExtents = HalfExtents};
        Core::Transform transform{.position = {0.0f, 3.0f, 0.0f}};

        // Two seconds are more than enough to fall 2 meters.
        for (int tick = 0; tick < 120; ++tick)
            UpdateCharacter(body, transform, brushes, PhysicsSettings{}, TickDuration);

        // Standing on the floor: the bottom of the box just above y = 0, not falling any more.
        EXPECT_TRUE(body.isOnGround);
        EXPECT_NEAR(transform.position.y, 0.875f, 0.01f);
        EXPECT_GT(transform.position.y - 0.875f, 0.0f);
        EXPECT_EQ(body.velocity.y, 0.0f);
    }

    TEST(CharacterMovement, MovingIntoWallAtAngleSlidesAlongIt)
    {
        // The box moves right (into the wall at x = 2) and forward (-z) at the same time: the wall stops the part to
        // the right, the part forward goes on.
        const std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        glm::dvec3 position{1.0, 1.0, 0.0};
        glm::vec3 velocity{6.0f, 0.0f, -6.0f};

        SlideMove(brushes, position, velocity, HalfExtents, 0.5f);

        EXPECT_NEAR(position.x, 1.5, 0.01);   // against the wall (its left side at 2, minus half the box)
        EXPECT_NEAR(position.z, -3.0, 0.01);  // slid the whole way forward: 6 m/s for 0.5 s
        EXPECT_NEAR(velocity.x, 0.0f, Tolerance);
        EXPECT_NEAR(velocity.z, -6.0f, Tolerance);
    }

    TEST(CharacterMovement, MovingIntoCornerStops)
    {
        // A second wall along x at z from -3 to -2 makes a corner with the first one; moving diagonally into it stops.
        std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        brushes.push_back(CreateBoxBrush({-50.0, 0.0, -3.0}, {50.0, 5.0, -2.0}));
        glm::dvec3 position{1.0, 1.0, -1.0};
        glm::vec3 velocity{6.0f, 0.0f, -6.0f};

        SlideMove(brushes, position, velocity, HalfExtents, 1.0f);

        EXPECT_NEAR(position.x, 1.5, 0.01);
        EXPECT_NEAR(position.z, -1.5, 0.01);
        EXPECT_EQ(velocity, glm::vec3(0.0f));
    }

    TEST(CharacterMovement, WalkableSlopeIsGroundButSteepSlopeIsNot)
    {
        // A ramp surface at 30 degrees (normal y = cos 30 = 0.87) and at 60 degrees (normal y = 0.5) under the box.
        const auto createSlope = [](double angleDegrees)
        {
            const double angle = angleDegrees * 3.14159265358979 / 180.0;
            World::CollisionBrush slope = CreateBoxBrush({-10.0, -10.0, -10.0}, {10.0, 0.0, 10.0});
            slope.planes[2] = {.normal = {std::sin(angle), std::cos(angle), 0.0}, .distance = 0.0}; // replaces the top
            return std::vector<World::CollisionBrush>{slope};
        };

        // The box stands on the slope at x = 0, where the surface passes through the origin.
        const auto isOnGroundAt = [](const std::vector<World::CollisionBrush>& brushes, double angleDegrees)
        {
            const double angle = angleDegrees * 3.14159265358979 / 180.0;
            const glm::dvec3 normal{std::sin(angle), std::cos(angle), 0.0};
            const double reach = std::abs(normal.x) * HalfExtents.x + std::abs(normal.y) * HalfExtents.y;
            const glm::dvec3 position = normal * (reach + World::SurfaceEpsilon);
            return IsOnGround(brushes, position, HalfExtents);
        };

        EXPECT_TRUE(isOnGroundAt(createSlope(30.0), 30.0));
        EXPECT_FALSE(isOnGroundAt(createSlope(60.0), 60.0));
    }
}
