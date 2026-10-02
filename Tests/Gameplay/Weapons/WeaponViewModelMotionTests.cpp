#include "Gameplay/Weapons/WeaponViewModelMotion.h"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace Abomination::Gameplay
{
    namespace
    {
        // Runs the motion for a number of frames of the same input.
        void RunFrames(WeaponViewModelMotion& motion, const WeaponViewModelMotionSettings& settings,
                       const WeaponViewModelMotionInput& input, int frameCount, float deltaTime)
        {
            for (int frame = 0; frame < frameCount; ++frame)
                UpdateWeaponViewModelMotion(motion, settings, input, deltaTime);
        }
    }

    TEST(WeaponViewModelMotion, StandingStillOnlyBreathes)
    {
        WeaponViewModelMotion motion;
        const WeaponViewModelMotionSettings settings;

        // Over two breaths the weapon moves, but never farther than the breath reaches.
        float largestOffset = 0.0f;
        for (int frame = 0; frame < 480; ++frame)
        {
            UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{}, 1.0f / 60.0f);
            largestOffset = std::max(largestOffset, glm::length(CalculateWeaponViewModelMotionOffset(motion, settings)));
        }

        EXPECT_GT(largestOffset, settings.idleAmount * 0.5f);
        EXPECT_LE(largestOffset, settings.idleAmount * 1.2f);
    }

    TEST(WeaponViewModelMotion, NoBreathingWhenIdleIsOff)
    {
        WeaponViewModelMotion motion;
        WeaponViewModelMotionSettings settings;
        settings.idleAmount = 0.0f;

        RunFrames(motion, settings, WeaponViewModelMotionInput{}, 120, 1.0f / 60.0f);

        EXPECT_NEAR(glm::length(CalculateWeaponViewModelMotionOffset(motion, settings)), 0.0f, 1e-6f);
    }

    TEST(WeaponViewModelMotion, RunningBobsAndStoppingFadesOut)
    {
        WeaponViewModelMotion motion;
        const WeaponViewModelMotionSettings settings;

        RunFrames(motion, settings, WeaponViewModelMotionInput{.horizontalSpeed = 7.0f}, 60, 1.0f / 60.0f);
        EXPECT_GT(motion.bobWeight, 0.9f);

        RunFrames(motion, settings, WeaponViewModelMotionInput{}, 120, 1.0f / 60.0f);
        EXPECT_LT(motion.bobWeight, 0.01f);
    }

    TEST(WeaponViewModelMotion, NoBobInTheAir)
    {
        WeaponViewModelMotion motion;
        const WeaponViewModelMotionSettings settings;

        RunFrames(motion, settings, WeaponViewModelMotionInput{.horizontalSpeed = 7.0f, .isOnGround = false}, 60,
                  1.0f / 60.0f);

        EXPECT_LT(motion.bobWeight, 0.01f);
    }

    TEST(WeaponViewModelMotion, TurningLeftLeavesWeaponToTheRightThenItCatchesUp)
    {
        WeaponViewModelMotion motion;
        const WeaponViewModelMotionSettings settings;
        UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{}, 1.0f / 60.0f);

        // A quick turn to the left (a bigger yaw) in one frame.
        UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{.look = {.yaw = 0.5f}}, 1.0f / 60.0f);
        EXPECT_GT(motion.sway.x, 0.0f);

        RunFrames(motion, settings, WeaponViewModelMotionInput{.look = {.yaw = 0.5f}}, 120, 1.0f / 60.0f);
        EXPECT_NEAR(motion.sway.x, 0.0f, 1e-4f);
    }

    TEST(WeaponViewModelMotion, SwayIsLimited)
    {
        WeaponViewModelMotion motion;
        const WeaponViewModelMotionSettings settings;
        UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{}, 1.0f / 60.0f);

        UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{.look = {.yaw = 100.0f}}, 1.0f / 60.0f);

        EXPECT_LE(glm::length(motion.sway), settings.swayMaximum);
    }

    TEST(WeaponViewModelMotion, LandingDipsAndSpringsBack)
    {
        WeaponViewModelMotion motion;
        const WeaponViewModelMotionSettings settings;

        // Falling at 8 m/s, then standing on the ground.
        UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{.isOnGround = false, .verticalSpeed = -8.0f},
                                    1.0f / 60.0f);
        UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{}, 1.0f / 60.0f);
        EXPECT_LT(motion.inertiaOffset, 0.0f);

        RunFrames(motion, settings, WeaponViewModelMotionInput{}, 120, 1.0f / 60.0f);
        EXPECT_NEAR(motion.inertiaOffset, 0.0f, 1e-4f);
    }

    TEST(WeaponViewModelMotion, JumpDipsTheWeapon)
    {
        WeaponViewModelMotion motion;
        const WeaponViewModelMotionSettings settings;

        UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{.isOnGround = false, .verticalSpeed = 8.4f},
                                    1.0f / 60.0f);

        EXPECT_LT(motion.inertiaOffset, 0.0f);
    }

    TEST(WeaponViewModelMotion, RecoilPushesBackTurnsUpAndReturns)
    {
        WeaponViewModelMotion motion;
        const WeaponViewModelMotionSettings settings;

        KickWeaponViewModelRecoil(motion, settings);
        RunFrames(motion, settings, WeaponViewModelMotionInput{}, 3, 1.0f / 60.0f);
        EXPECT_GT(motion.recoilBack, 0.0f);
        EXPECT_GT(motion.recoilPitch, 0.0f);

        RunFrames(motion, settings, WeaponViewModelMotionInput{}, 120, 1.0f / 60.0f);
        EXPECT_NEAR(motion.recoilBack, 0.0f, 1e-4f);
        EXPECT_NEAR(motion.recoilPitch, 0.0f, 1e-4f);
    }

    TEST(WeaponViewModelMotion, SameMotionAtAnyFrameRate)
    {
        // A landing seen at 30 and at 240 frames per second: 0.1 s after it the weapon is at the same place. The landing
        // frame itself moves the spring by one frame, so one frame fewer follows it.
        const WeaponViewModelMotionSettings settings;
        const auto landAndWait = [&](float deltaTime)
        {
            WeaponViewModelMotion motion;
            const WeaponViewModelMotionInput falling{.isOnGround = false, .verticalSpeed = -8.0f};
            UpdateWeaponViewModelMotion(motion, settings, falling, deltaTime);
            UpdateWeaponViewModelMotion(motion, settings, WeaponViewModelMotionInput{}, deltaTime);
            RunFrames(motion, settings, WeaponViewModelMotionInput{}, static_cast<int>(std::lround(0.1f / deltaTime)) - 1,
                      deltaTime);
            return motion.inertiaOffset;
        };

        EXPECT_NEAR(landAndWait(1.0f / 30.0f), landAndWait(1.0f / 240.0f), 0.002f);
    }
}
