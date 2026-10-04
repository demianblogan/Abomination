#include "Gameplay/Enemies/DogMind.h"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        constexpr float Tick = 1.0f / 60.0f;

        // A dog at the origin facing -Z, the player in front of it at distance, in plain sight.
        DogPerception PlayerInFront(float distance)
        {
            return DogPerception{.playerPosition = {0.0f, 0.0f, -distance}, .hasLineOfSight = true};
        }

        // The player behind the dog at distance: out of its field of view.
        DogPerception PlayerBehind(float distance)
        {
            return DogPerception{.playerPosition = {0.0f, 0.0f, distance}, .hasLineOfSight = true};
        }

        DogDecision Step(DogMind& mind, const DogPerception& perception, const DogSettings& settings, Core::Random& random)
        {
            return UpdateDogMind(mind, perception, settings, Tick, random);
        }

        // Runs the mind for seconds with the same perception, returning the last decision.
        DogDecision RunMind(DogMind& mind, const DogPerception& perception, const DogSettings& settings, float seconds,
                            Core::Random& random)
        {
            DogDecision decision;
            for (float time = 0.0f; time < seconds; time += Tick)
                decision = Step(mind, perception, settings, random);
            return decision;
        }
    }

    TEST(DogMind, SeesPlayerInFrontNotBehind)
    {
        const DogSettings settings;

        EXPECT_TRUE(CanSeePlayer(PlayerInFront(5.0f), settings));
        EXPECT_FALSE(CanSeePlayer(PlayerBehind(5.0f), settings));
    }

    TEST(DogMind, WallHidesPlayer)
    {
        DogPerception perception = PlayerInFront(5.0f);
        perception.hasLineOfSight = false;

        EXPECT_FALSE(CanSeePlayer(perception, DogSettings{}));
    }

    TEST(DogMind, SmellsPlayerCloseBehind)
    {
        DogPerception perception = PlayerBehind(2.0f);
        perception.hasLineOfSight = false;

        EXPECT_TRUE(NoticesPlayer(perception, DogSettings{}));
        EXPECT_FALSE(NoticesPlayer(PlayerBehind(10.0f), DogSettings{}));
    }

    TEST(DogMind, NoticingPlayerAlertsThenChases)
    {
        DogMind mind;
        Core::Random random(1);
        const DogSettings settings;

        static_cast<void>(Step(mind, PlayerInFront(10.0f), settings, random));
        EXPECT_EQ(mind.state, DogState::Alert);

        const DogDecision decision = RunMind(mind, PlayerInFront(10.0f), settings, settings.alertTime + 0.1f, random);
        EXPECT_EQ(mind.state, DogState::Chase);
        EXPECT_EQ(decision.animation, "Gallop");
        EXPECT_FLOAT_EQ(decision.speed, settings.runSpeed);
        EXPECT_LT(decision.faceDirection.z, -0.99f);
    }

    TEST(DogMind, ShotHeardBehindAlerts)
    {
        DogMind mind;
        Core::Random random(2);
        DogPerception perception = PlayerBehind(10.0f);
        perception.hasShotBeenFired = true;
        perception.shotPosition = perception.playerPosition;

        static_cast<void>(Step(mind, perception, DogSettings{}, random));

        EXPECT_EQ(mind.state, DogState::Alert);
    }

    TEST(DogMind, ChasesPlayerOutOfSightUntilFarEnough)
    {
        DogMind mind{.state = DogState::Chase};
        Core::Random random(3);
        const DogSettings settings;
        DogPerception perception = PlayerBehind(10.0f);
        perception.hasLineOfSight = false;

        static_cast<void>(RunMind(mind, perception, settings, 3.0f, random));
        EXPECT_EQ(mind.state, DogState::Chase);

        perception.playerPosition.z = settings.loseDistance + 1.0f;
        static_cast<void>(Step(mind, perception, settings, random));
        EXPECT_NE(mind.state, DogState::Chase);
    }

    TEST(DogMind, LeapsFromAFewMetersAndMissesIfPlayerStaysAway)
    {
        DogMind mind{.state = DogState::Chase};
        Core::Random random(4);
        const DogSettings settings;
        const DogPerception perception = PlayerInFront(3.0f);

        const DogDecision leap = Step(mind, perception, settings, random);
        EXPECT_EQ(mind.state, DogState::Leap);
        EXPECT_EQ(leap.animation, "Gallop_Jump");
        EXPECT_LT(leap.impulse.z, 0.0f);
        EXPECT_GT(leap.impulse.y, 0.0f);

        // The player is still 3 m away when the teeth close: the leap misses (the bite reaches 1.7 m).
        int bites = 0;
        for (float time = 0.0f; time < settings.leapDuration - 0.05f; time += Tick)
            bites += Step(mind, perception, settings, random).bites ? 1 : 0;
        EXPECT_EQ(bites, 0);
    }

    TEST(DogMind, BitesStandingNextToPlayer)
    {
        DogMind mind{.state = DogState::Chase};
        Core::Random random(5);
        const DogSettings settings;
        const DogPerception perception = PlayerInFront(1.0f);

        const DogDecision first = Step(mind, perception, settings, random);
        EXPECT_EQ(mind.state, DogState::Bite);
        EXPECT_EQ(first.animation, "Attack");
        EXPECT_FLOAT_EQ(first.speed, 0.0f);

        int bites = 0;
        for (float time = 0.0f; time < settings.biteDuration - 0.05f; time += Tick)
            bites += Step(mind, perception, settings, random).bites ? 1 : 0;
        EXPECT_EQ(bites, 1);
    }

    TEST(DogMind, HitMakesItFlinchThenChase)
    {
        DogMind mind;
        Core::Random random(6);
        const DogSettings settings;
        DogPerception perception = PlayerBehind(10.0f);
        perception.wasHurt = true;

        const DogDecision decision = Step(mind, perception, settings, random);
        EXPECT_EQ(mind.state, DogState::Pain);
        EXPECT_EQ(decision.animation, "Idle_HitReact1");

        perception.wasHurt = false;
        static_cast<void>(RunMind(mind, perception, settings, settings.painTime + 0.1f, random));
        EXPECT_EQ(mind.state, DogState::Chase);
    }

    TEST(DogMind, PatrolTurnsBeforeWalkingAndStaysNearHome)
    {
        DogMind mind;
        Core::Random random(7);
        const DogSettings settings;
        DogPerception perception = PlayerBehind(20.0f);
        perception.home = {50.0f, 0.0f, 0.0f};

        static_cast<void>(RunMind(mind, perception, settings, mind.idleDuration + 0.05f, random));
        ASSERT_EQ(mind.state, DogState::Patrol);
        EXPECT_LE(glm::length(mind.patrolTarget - perception.home), settings.patrolRadius + 1e-4f);

        // The target is far to the side (around home, 50 m along +X): it turns on the spot first.
        const DogDecision decision = Step(mind, perception, settings, random);
        EXPECT_FLOAT_EQ(decision.speed, 0.0f);
        EXPECT_GT(decision.faceDirection.x, 0.9f);
    }

    TEST(DogMind, BlockedPatrolStops)
    {
        DogMind mind{.state = DogState::Patrol, .patrolTarget = {0.0f, 0.0f, -5.0f}};
        Core::Random random(8);
        DogPerception perception = PlayerBehind(20.0f);
        perception.isBlocked = true;

        static_cast<void>(Step(mind, perception, DogSettings{}, random));

        EXPECT_EQ(mind.state, DogState::Idle);
    }
}
