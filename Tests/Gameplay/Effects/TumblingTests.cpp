#include "Gameplay/Effects/Tumbling.h"

#include "World/CollisionBrush.h"

#include <gtest/gtest.h>

#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        // A floor with its top at y = 0.
        std::vector<World::CollisionBrush> CreateFloor()
        {
            return {World::CreateBoxCollisionBrush({0.0, -1.0, 0.0}, {50.0, 1.0, 50.0})};
        }
    }

    TEST(Tumbling, DroppedThingBouncesThenLiesOnTheFloor)
    {
        const std::vector<World::CollisionBrush> floor = CreateFloor();
        Tumbler tumbler{.velocity = {1.0f, 0.0f, 0.0f}, .spinSpeed = 10.0f};
        Core::Transform transform{.position = {0.0f, 1.0f, 0.0f}};
        const BounceSettings settings;

        int hits = 0;
        bool hasStopped = false;
        for (int step = 0; step < 600 && !hasStopped; ++step)
        {
            const TumbleStep result = Tumble(tumbler, transform, floor, 25.0f, 0.01, settings, 1.0f / 60.0f);
            hits += result.hasHit ? 1 : 0;
            hasStopped = result.hasStopped;
        }

        // It bounced a few times, lost its speed and lies on the floor, still.
        EXPECT_TRUE(hasStopped);
        EXPECT_GE(hits, 2);
        EXPECT_NEAR(transform.position.y, 0.01f, 0.01f);
        EXPECT_EQ(tumbler.velocity, glm::vec3(0.0f));
    }

    TEST(Tumbling, BounceTurnsBackThePartIntoTheFloor)
    {
        const std::vector<World::CollisionBrush> floor = CreateFloor();
        Tumbler tumbler{.velocity = {2.0f, -10.0f, 0.0f}};
        Core::Transform transform{.position = {0.0f, 0.05f, 0.0f}};
        const BounceSettings settings{.bounce = 0.5f, .slide = 0.5f};

        const TumbleStep step = Tumble(tumbler, transform, floor, 0.0f, 0.01, settings, 0.1f);

        // 10 m/s into the floor: back up at half of it; 2 m/s along it, slowed to half.
        ASSERT_TRUE(step.hasHit);
        EXPECT_FLOAT_EQ(step.speedIntoSurface, 10.0f);
        EXPECT_FLOAT_EQ(tumbler.velocity.y, 5.0f);
        EXPECT_FLOAT_EQ(tumbler.velocity.x, 1.0f);
    }
}
