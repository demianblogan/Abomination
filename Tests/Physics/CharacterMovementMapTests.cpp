#include "Physics/CharacterMovement.h"
#include "World/CollisionBrush.h"
#include "World/MapParser.h"
#include "World/PlayerStart.h"

#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <expected>
#include <random>
#include <string>
#include <string_view>
#include <vector>

// Movement on real level geometry: part of the test map of 2026-09-27 (the walls and floor of the room, the raised floor
// and the two octagonal columns). The tests with simple boxes did not find three bugs that running around this map did,
// because they need slanted faces, floats rounded near walls and moves that end a hair in front of a surface.
namespace Abomination::Physics
{
    namespace
    {
        constexpr std::string_view TestMapPart = R"MAP(
{
"classname" "worldspawn"
// brush 0
{
( 48 64 240 ) ( 48 -448 240 ) ( 48 64 -16 ) Episode1/Wall_MossyBrick [ 0 1 0 0 ] [ 0 0 -1 0 ] 270 1 1
( 64 -448 -16 ) ( 48 -448 -16 ) ( 64 -448 240 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 0 -1 0 ] 180 1 1
( 64 64 -16 ) ( 48 64 -16 ) ( 64 -448 -16 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 64 -448 240 ) ( 48 -448 240 ) ( 64 64 240 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 64 64 240 ) ( 48 64 240 ) ( 64 64 -16 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 64 -448 240 ) ( 64 64 240 ) ( 64 -448 -16 ) Episode1/Wall_MossyBrick [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
}
// brush 1
{
( -448 -448 -16 ) ( -448 64 -16 ) ( -448 -448 240 ) Episode1/Wall_MossyBrick [ 0 -1 0 0 ] [ 0 0 -1 0 ] 90 1 1
( -448 -448 240 ) ( -432 -448 240 ) ( -448 -448 -16 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 0 -1 0 ] 180 1 1
( -448 -448 -16 ) ( -432 -448 -16 ) ( -448 64 -16 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( -448 64 240 ) ( -432 64 240 ) ( -448 -448 240 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( -448 64 -16 ) ( -432 64 -16 ) ( -448 64 240 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -432 -448 -16 ) ( -432 -448 240 ) ( -432 64 -16 ) Episode1/Wall_MossyBrick [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 1 1
}
// brush 2
{
( -432 64 240 ) ( -432 48 240 ) ( -432 64 -16 ) Episode1/Wall_MossyBrick [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 48 48 240 ) ( 48 48 -16 ) ( -432 48 240 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 0 -1 0 ] 270 1 1
( -432 64 -16 ) ( -432 48 -16 ) ( 48 64 -16 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 48 64 240 ) ( 48 48 240 ) ( -432 64 240 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( 48 64 240 ) ( -432 64 240 ) ( 48 64 -16 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 48 64 -16 ) ( 48 48 -16 ) ( 48 64 240 ) Episode1/Wall_MossyBrick [ 0 1 0 0 ] [ 0 0 -1 0 ] 270 1 1
}
// brush 3
{
( -432 -448 -16 ) ( -432 -432 -16 ) ( -432 -448 240 ) Episode1/Wall_MossyBrick [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( 48 -448 -16 ) ( -432 -448 -16 ) ( 48 -448 240 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 0 -1 0 ] 180 1 1
( 48 -448 -16 ) ( 48 -432 -16 ) ( -432 -448 -16 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( -432 -448 240 ) ( -432 -432 240 ) ( 48 -448 240 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( -432 -432 -16 ) ( 48 -432 -16 ) ( -432 -432 240 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 0 -1 0 ] 90 1 1
( 48 -448 240 ) ( 48 -432 240 ) ( 48 -448 -16 ) Episode1/Wall_MossyBrick [ 0 1 0 0 ] [ 0 0 -1 0 ] 270 1 1
}
// brush 4
{
( -432 48 -16 ) ( -432 48 0 ) ( -432 -432 -16 ) Episode1/Floor_WetFlagstone [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -432 -432 -16 ) ( -432 -432 0 ) ( 48 -432 -16 ) Episode1/Floor_WetFlagstone [ 1 0 0 0 ] [ 0 0 -1 0 ] 90 1 1
( -432 48 -16 ) ( -432 -432 -16 ) ( 48 48 -16 ) Episode1/Floor_WetFlagstone [ -1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( -432 -432 0 ) ( -432 48 0 ) ( 48 -432 0 ) Episode1/Floor_WetFlagstone [ -1 0 0 0 ] [ 0 -1 0 0 ] 270 1 1
( 48 48 -16 ) ( 48 48 0 ) ( -432 48 -16 ) Episode1/Floor_WetFlagstone [ -1 0 0 0 ] [ 0 0 -1 0 ] 270 1 1
( 48 -432 -16 ) ( 48 -432 0 ) ( 48 48 -16 ) Episode1/Floor_WetFlagstone [ 0 1 0 0 ] [ 0 0 -1 0 ] 270 1 1
}
// brush 5
{
( -448 -64 0 ) ( -448 -63 0 ) ( -448 -64 1 ) Episode1/Floor_WetFlagstone [ 0 -1 0 0 ] [ 0 0 -1 0 ] 270 1 1
( -384 -448 0 ) ( -384 -448 1 ) ( -383 -448 0 ) Episode1/Floor_WetFlagstone [ 1 0 0 0 ] [ 0 0 -1 0 ] 90 1 1
( -384 -64 0 ) ( -383 -64 0 ) ( -384 -63 0 ) Episode1/Floor_WetFlagstone [ -1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( -320 0 64 ) ( -320 1 64 ) ( -319 0 64 ) Episode1/Floor_WetFlagstone [ 1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( -320 64 64 ) ( -319 64 64 ) ( -320 64 65 ) Episode1/Floor_WetFlagstone [ -1 0 0 0 ] [ 0 0 -1 0 ] 270 1 1
( -192 0 64 ) ( -192 0 65 ) ( -192 1 64 ) Episode1/Floor_WetFlagstone [ 0 1 0 0 ] [ 0 0 -1 0 ] 270 1 1
}
// brush 10
{
( -320 -109.25483399593905 128 ) ( -320 -82.74516600406093 64 ) ( -320 -82.74516600406093 128 ) Episode1/Wall_MossyBrick [ 0 -1 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -301.25483399593907 -128 128 ) ( -320 -109.25483399593905 64 ) ( -320 -109.25483399593905 128 ) Episode1/Wall_MossyBrick [ 0.7071067811865474 -0.7071067811865478 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -301.25483399593907 -64 128 ) ( -320 -82.74516600406093 64 ) ( -301.25483399593907 -64 64 ) Episode1/Wall_MossyBrick [ -0.7071067811865476 -0.7071067811865476 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -274.74516600406093 -128 128 ) ( -301.25483399593907 -128 64 ) ( -301.25483399593907 -128 128 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -256 -82.74516600406096 64 ) ( -274.74516600406093 -128 64 ) ( -256 -109.25483399593904 64 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 -1 0 0 ] 0 1 1
( -256 -82.74516600406096 256 ) ( -301.25483399593907 -64 256 ) ( -274.74516600406093 -64 256 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 -1 0 0 ] 270 1 1
( -274.74516600406093 -64 128 ) ( -301.25483399593907 -64 64 ) ( -274.74516600406093 -64 64 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -256 -109.25483399593904 128 ) ( -274.74516600406093 -128 64 ) ( -274.74516600406093 -128 128 ) Episode1/Wall_MossyBrick [ 0.707106781186547 0.7071067811865481 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -256 -82.74516600406096 128 ) ( -274.74516600406093 -64 64 ) ( -256 -82.74516600406096 64 ) Episode1/Wall_MossyBrick [ -0.707106781186547 0.7071067811865481 0 0 ] [ 0 0 -1 0 ] 0 1 1
( -256 -82.74516600406096 128 ) ( -256 -109.25483399593904 64 ) ( -256 -109.25483399593904 128 ) Episode1/Wall_MossyBrick [ 0 1 0 0 ] [ 0 0 -1 0 ] 0 1 1
}
// brush 11
{
( -320 -301.25483399593907 128 ) ( -320 -274.74516600406093 64 ) ( -320 -274.74516600406093 128 ) Episode1/Wall_MossyBrick [ 0 -1 0 0 ] [ 0 0 -1 0 ] 270 1 1
( -301.25483399593907 -320 128 ) ( -320 -301.25483399593907 64 ) ( -320 -301.25483399593907 128 ) Episode1/Wall_MossyBrick [ 0.7071067811865474 -0.7071067811865478 0 -7.764496 ] [ 0 0 -1 0 ] 270 1 1
( -301.25483399593907 -256 128 ) ( -320 -274.74516600406093 64 ) ( -301.25483399593907 -256 64 ) Episode1/Wall_MossyBrick [ -0.7071067811865476 -0.7071067811865476 0 -7.7645264 ] [ 0 0 -1 0 ] 270 1 1
( -274.74516600406093 -320 128 ) ( -301.25483399593907 -320 64 ) ( -301.25483399593907 -320 128 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 0 -1 0 ] 270 1 1
( -256 -274.74516600406093 64 ) ( -274.74516600406093 -320 64 ) ( -256 -301.25483399593907 64 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 -1 0 0 ] 270 1 1
( -256 -274.74516600406093 256 ) ( -301.25483399593907 -256 256 ) ( -274.74516600406093 -256 256 ) Episode1/Wall_MossyBrick [ 1 0 0 0 ] [ 0 -1 0 0 ] 180 1 1
( -274.74516600406093 -256 128 ) ( -301.25483399593907 -256 64 ) ( -274.74516600406093 -256 64 ) Episode1/Wall_MossyBrick [ -1 0 0 0 ] [ 0 0 -1 0 ] 270 1 1
( -256 -301.25483399593907 128 ) ( -274.74516600406093 -320 64 ) ( -274.74516600406093 -320 128 ) Episode1/Wall_MossyBrick [ 0.707106781186547 0.7071067811865481 0 7.7645264 ] [ 0 0 -1 0 ] 270 1 1
( -256 -274.74516600406093 128 ) ( -274.74516600406093 -256 64 ) ( -256 -274.74516600406093 64 ) Episode1/Wall_MossyBrick [ -0.707106781186547 0.7071067811865481 0 7.764496 ] [ 0 0 -1 0 ] 270 1 1
( -256 -274.74516600406093 128 ) ( -256 -301.25483399593907 64 ) ( -256 -301.25483399593907 128 ) Episode1/Wall_MossyBrick [ 0 1 0 0 ] [ 0 0 -1 0 ] 270 1 1
}
}
)MAP";

        std::vector<World::CollisionBrush> LoadBrushes()
        {
            const std::expected<World::MapData, std::string> map = World::ParseMap(TestMapPart);
            EXPECT_TRUE(map.has_value());

            return World::BuildCollisionBrushes(map->entities.front());
        }
    }

    // A random walk: the player runs in a new random direction every third of a second and jumps now and then, for many
    // ticks, and must never end a tick inside a brush or with a velocity that is not a number. The random numbers come
    // from a fixed seed, so every run of the test is the same walk.
    //
    // It caught three bugs, all fixed in the review of 0.2:
    //   - TraceBox skipped brushes whose bounding box the way did not overlap, even when the way ended closer to them
    //     than SurfaceEpsilon; the box was then rounded to float onto the wall and counted as inside it;
    //   - TraceBox ignored a wall when the box started closer to it than SurfaceEpsilon and the entering fraction came
    //     out below -1;
    //   - SlideMove remembered one slanted face twice and took the cross product of its normal with itself: NaN.
    TEST(CharacterMovementOnMap, RandomWalkNeverEndsInsideBrushOrWithNaN)
    {
        const std::vector<World::CollisionBrush> brushes = LoadBrushes();
        ASSERT_EQ(brushes.size(), 8u);

        std::mt19937 random(12345);
        std::uniform_real_distribution<float> unit(0.0f, 1.0f);
        constexpr float TickDuration = 1.0f / 60.0f;
        int solidTickCount = 0;
        int notANumberTickCount = 0;

        for (int run = 0; run < 40; ++run)
        {
            CharacterBody body{.halfExtents = World::PlayerHalfExtents};
            Core::Transform transform{.position = {-7.0f, 3.5f, 6.0f}};
            MoveCommand command;
            for (int tick = 0; tick < 1500; ++tick)
            {
                if (tick % 20 == 0)
                {
                    const float angle = unit(random) * 6.2831853f;
                    command.wishDirection = glm::vec3(std::cos(angle), 0.0f, std::sin(angle));
                }
                command.wantsToJump = unit(random) < 0.05f;

                UpdateCharacter(body, transform, brushes, PhysicsSettings{}, MovementSettings{}, command, TickDuration);

                const glm::vec3& position = transform.position;
                if (std::isnan(position.x) || std::isnan(position.y) || std::isnan(position.z) ||
                    std::isnan(body.velocity.x) || std::isnan(body.velocity.y) || std::isnan(body.velocity.z))
                {
                    ++notANumberTickCount;
                    break;
                }

                if (IsInSolid(brushes, glm::dvec3(position), body.halfExtents))
                    ++solidTickCount;
            }
        }

        EXPECT_EQ(notANumberTickCount, 0);
        EXPECT_EQ(solidTickCount, 0);
    }
}
