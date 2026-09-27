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

        UpdateCharacter(body, transform, CreateFloorAndWall(), PhysicsSettings{.gravity = 25.0f}, MovementSettings{}, {},
                        TickDuration);

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
            UpdateCharacter(body, transform, brushes, PhysicsSettings{}, MovementSettings{}, {}, TickDuration);

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

    TEST(CharacterMovement, FrictionStopsCharacterWithoutInput)
    {
        glm::vec3 velocity{5.0f, 0.0f, 0.0f};
        const MovementSettings settings;

        // Friction takes friction * speed per second: 4 * 5 = 20 m/s per second, so after one tick of 1/60 s the speed
        // is 5 - 20/60.
        ApplyFriction(velocity, settings, TickDuration);
        EXPECT_NEAR(velocity.x, 5.0f - 20.0f * TickDuration, Tolerance);

        // A second later the character stands still.
        for (int tick = 0; tick < 60; ++tick)
            ApplyFriction(velocity, settings, TickDuration);
        EXPECT_EQ(velocity, glm::vec3(0.0f));
    }

    TEST(CharacterMovement, AccelerateAddsAtMostWhatIsMissing)
    {
        // Standing: one tick adds acceleration * wishSpeed * dt = 10 * 10 / 60.
        glm::vec3 velocity{0.0f};
        Accelerate(velocity, {1.0f, 0.0f, 0.0f}, 10.0f, 10.0f, TickDuration);
        EXPECT_NEAR(velocity.x, 100.0f * TickDuration, Tolerance);

        // Already at the wished speed in that direction: nothing is added.
        velocity = {10.0f, 0.0f, 0.0f};
        Accelerate(velocity, {1.0f, 0.0f, 0.0f}, 10.0f, 10.0f, TickDuration);
        EXPECT_NEAR(velocity.x, 10.0f, Tolerance);
    }

    TEST(CharacterMovement, WalkingOnFloorReachesMaximumSpeed)
    {
        const std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        CharacterBody body{.halfExtents = HalfExtents, .isOnGround = true};
        Core::Transform transform{.position = {0.0f, 0.875f + static_cast<float>(World::SurfaceEpsilon), 0.0f}};
        const MoveCommand command{.wishDirection = {0.0f, 0.0f, -1.0f}};

        // Friction and acceleration settle on the speed where they balance: with these values that is the maximum speed
        // (acceleration adds 10 * maxSpeed per second, more than friction takes, 4 * speed).
        for (int tick = 0; tick < 60; ++tick)
            UpdateCharacter(body, transform, brushes, PhysicsSettings{}, MovementSettings{}, command, TickDuration);

        EXPECT_TRUE(body.isOnGround);
        const float maxSpeed = MovementSettings{}.maxSpeed;
        EXPECT_GT(-body.velocity.z, 0.8f * maxSpeed);
        EXPECT_LE(-body.velocity.z, maxSpeed + Tolerance);
        EXPECT_LT(transform.position.z, -4.0f);
    }

    TEST(CharacterMovement, StepUpWalksOntoLowStep)
    {
        // A step 0.4 m high (lower than the 0.56 m step height) in front of the character.
        std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        brushes.push_back(CreateBoxBrush({-50.0, 0.0, -50.0}, {50.0, 0.4, -2.0}));
        glm::dvec3 position{0.0, 0.875 + World::SurfaceEpsilon, 0.0};
        glm::vec3 velocity{0.0f, 0.0f, -8.0f};

        const float steppedUpHeight = StepSlideMove(brushes, position, velocity, HalfExtents, 18.0f / 32.0f, 0.5f);

        // Went over the edge at z = -2 and stands on the step.
        EXPECT_NEAR(steppedUpHeight, 0.4f, 0.01f);
        EXPECT_LT(position.z, -2.5);
        EXPECT_NEAR(position.y, 0.4 + 0.875, 0.01);
    }

    TEST(CharacterMovement, StepUpDoesNotClimbHighWall)
    {
        // A block 1 m high: too high for a step, the character stops at it.
        std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        brushes.push_back(CreateBoxBrush({-50.0, 0.0, -50.0}, {50.0, 1.0, -2.0}));
        glm::dvec3 position{0.0, 0.875 + World::SurfaceEpsilon, 0.0};
        glm::vec3 velocity{0.0f, 0.0f, -8.0f};

        StepSlideMove(brushes, position, velocity, HalfExtents, 18.0f / 32.0f, 0.5f);

        EXPECT_NEAR(position.z, -1.5, 0.01); // stopped half the box before the block
        EXPECT_NEAR(position.y, 0.875, 0.01);
    }

    TEST(CharacterMovement, BoxSlightlyInWallIsPushedOut)
    {
        // The box reaches 2 mm into the wall at x = 2: a case like the player once stuck at a wall of the test map.
        const std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        glm::dvec3 position{1.502, 1.0, 0.0};
        ASSERT_TRUE(IsInSolid(brushes, position, HalfExtents));

        EXPECT_TRUE(PushOutOfSolid(brushes, position, HalfExtents));

        EXPECT_FALSE(IsInSolid(brushes, position, HalfExtents));
        EXPECT_NEAR(position.x, 1.502, 0.04); // moved by a few millimeters at most
    }

    TEST(CharacterMovement, BoxDeepInWallIsNotPushedOut)
    {
        // Half a meter inside: too far for the few millimeters PushOutOfSolid tries. This is not its job.
        const std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        glm::dvec3 position{2.5, 2.0, 0.0};

        EXPECT_FALSE(PushOutOfSolid(brushes, position, HalfExtents));
        EXPECT_EQ(position, glm::dvec3(2.5, 2.0, 0.0));
    }

    TEST(CharacterMovement, StuckCharacterIsPushedOutAndMarked)
    {
        const std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        CharacterBody body{.halfExtents = HalfExtents, .isOnGround = true};
        Core::Transform transform{.position = {1.502f, 0.9f, 0.0f}};

        UpdateCharacter(body, transform, brushes, PhysicsSettings{}, MovementSettings{}, {}, TickDuration);

        EXPECT_TRUE(body.isInSolid); // started the tick inside the wall
        EXPECT_FALSE(IsInSolid(brushes, glm::dvec3(transform.position), HalfExtents));
    }

    TEST(CharacterMovement, JumpLeavesGroundAndLandsAgain)
    {
        const std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        CharacterBody body{.halfExtents = HalfExtents, .isOnGround = true};
        const float standingY = 0.875f + static_cast<float>(World::SurfaceEpsilon);
        Core::Transform transform{.position = {0.0f, standingY, 0.0f}};
        const MovementSettings settings;

        UpdateCharacter(body, transform, brushes, PhysicsSettings{}, settings, MoveCommand{.wantsToJump = true},
                        TickDuration);
        EXPECT_FALSE(body.isOnGround);
        EXPECT_GT(body.velocity.y, 0.0f);

        // The highest point: jumpSpeed² / (2 × gravity) = 8.44² / 50 = about 1.42 m.
        float highestY = transform.position.y;
        for (int tick = 0; tick < 120 && !body.isOnGround; ++tick)
        {
            UpdateCharacter(body, transform, brushes, PhysicsSettings{}, settings, {}, TickDuration);
            highestY = glm::max(highestY, transform.position.y);
        }

        EXPECT_NEAR(highestY - standingY, 1.42f, 0.1f);
        EXPECT_TRUE(body.isOnGround);
        EXPECT_NEAR(transform.position.y, standingY, 0.01f);
    }

    TEST(CharacterMovement, NoJumpInAir)
    {
        CharacterBody body{.halfExtents = HalfExtents};
        Core::Transform transform{.position = {0.0f, 5.0f, 0.0f}};

        UpdateCharacter(body, transform, CreateFloorAndWall(), PhysicsSettings{}, MovementSettings{},
                        MoveCommand{.wantsToJump = true}, TickDuration);

        EXPECT_LT(body.velocity.y, 0.0f); // still falling
    }

    TEST(CharacterMovement, AirControlAddsOnlyALittleSpeed)
    {
        // Falling straight down and wishing to go right: at most maxAirWishSpeed (about 1 m/s) is added that way.
        glm::vec3 velocity{0.0f, -5.0f, 0.0f};
        const MovementSettings settings;

        for (int tick = 0; tick < 60; ++tick)
            AirAccelerate(velocity, {1.0f, 0.0f, 0.0f}, settings.maxSpeed, settings.maxAirWishSpeed,
                          settings.airAcceleration, TickDuration);

        EXPECT_NEAR(velocity.x, settings.maxAirWishSpeed, Tolerance);
    }

    TEST(CharacterMovement, LandingOnStairsKeepsRunning)
    {
        // Falling forward towards a step 0.4 m high: the bottom of the box is 0.3 m above the floor, lower than the top of
        // the step, so the edge of the step is in the way. The box steps over it and runs on instead of stopping.
        std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        brushes.push_back(CreateBoxBrush({-50.0, 0.0, -50.0}, {50.0, 0.4, -2.0}));
        CharacterBody body{.halfExtents = HalfExtents, .velocity = {0.0f, -1.0f, -8.0f}};
        Core::Transform transform{.position = {0.0f, 0.875f + 0.3f, -1.0f}};

        for (int tick = 0; tick < 30; ++tick)
            UpdateCharacter(body, transform, brushes, PhysicsSettings{}, MovementSettings{}, {}, TickDuration);

        EXPECT_LT(transform.position.z, -2.5f);                  // got past the edge at z = -2
        EXPECT_NEAR(transform.position.y, 0.4f + 0.875f, 0.02f); // stands on the step
    }

    TEST(CharacterMovement, RisingCharacterDoesNotStepUp)
    {
        // Moving up (the first half of a jump) into the face of a step: no stepping, the face stops the box like a wall.
        std::vector<World::CollisionBrush> brushes = CreateFloorAndWall();
        brushes.push_back(CreateBoxBrush({-50.0, 0.0, -50.0}, {50.0, 0.4, -2.0}));
        CharacterBody body{.halfExtents = HalfExtents, .velocity = {0.0f, 1.0f, -8.0f}};
        Core::Transform transform{.position = {0.0f, 0.875f + 0.05f, -1.45f}}; // one tick (0.13 m) reaches the face

        UpdateCharacter(body, transform, brushes, PhysicsSettings{}, MovementSettings{}, {}, TickDuration);

        EXPECT_NEAR(transform.position.z, -1.5f, 0.01f); // stopped at the face of the step
        EXPECT_EQ(body.steppedUpHeight, 0.0f);
    }
}
