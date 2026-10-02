#include "Gameplay/Weapons/ViewModelMotion.h"

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace Abomination::Gameplay
{
    namespace
    {
        // Runs the motion for a number of frames of the same input.
        void RunFrames(ViewModelMotion& motion, const ViewModelMotionSettings& settings, const ViewModelMotionInput& input,
                 int frameCount, float deltaTime)
        {
            for (int frame = 0; frame < frameCount; ++frame)
                UpdateViewModelMotion(motion, settings, input, deltaTime);
        }
    }

    TEST(ViewModelMotion, StandingStillOnlyBreathes)
    {
        ViewModelMotion motion;
        const ViewModelMotionSettings settings;

        // Over two breaths the weapon moves, but never farther than the breath reaches.
        float largestOffset = 0.0f;
        for (int frame = 0; frame < 480; ++frame)
        {
            UpdateViewModelMotion(motion, settings, ViewModelMotionInput{}, 1.0f / 60.0f);
            largestOffset = std::max(largestOffset, glm::length(CalculateViewModelMotionOffset(motion, settings)));
        }

        EXPECT_GT(largestOffset, settings.idleAmount * 0.5f);
        EXPECT_LE(largestOffset, settings.idleAmount * 1.2f);
    }

    TEST(ViewModelMotion, NoBreathingWhenIdleIsOff)
    {
        ViewModelMotion motion;
        ViewModelMotionSettings settings;
        settings.idleAmount = 0.0f;

        RunFrames(motion, settings, ViewModelMotionInput{}, 120, 1.0f / 60.0f);

        EXPECT_NEAR(glm::length(CalculateViewModelMotionOffset(motion, settings)), 0.0f, 1e-6f);
    }

    TEST(ViewModelMotion, RunningBobsAndStoppingFadesOut)
    {
        ViewModelMotion motion;
        const ViewModelMotionSettings settings;

        RunFrames(motion, settings, ViewModelMotionInput{.horizontalSpeed = 7.0f}, 60, 1.0f / 60.0f);
        EXPECT_GT(motion.bobWeight, 0.9f);

        RunFrames(motion, settings, ViewModelMotionInput{}, 120, 1.0f / 60.0f);
        EXPECT_LT(motion.bobWeight, 0.01f);
    }

    TEST(ViewModelMotion, NoBobInTheAir)
    {
        ViewModelMotion motion;
        const ViewModelMotionSettings settings;

        RunFrames(motion, settings, ViewModelMotionInput{.horizontalSpeed = 7.0f, .isOnGround = false}, 60, 1.0f / 60.0f);

        EXPECT_LT(motion.bobWeight, 0.01f);
    }

    TEST(ViewModelMotion, TurningLeftLeavesWeaponToTheRightThenItCatchesUp)
    {
        ViewModelMotion motion;
        const ViewModelMotionSettings settings;
        UpdateViewModelMotion(motion, settings, ViewModelMotionInput{}, 1.0f / 60.0f);

        // A quick turn to the left (a bigger yaw) in one frame.
        UpdateViewModelMotion(motion, settings, ViewModelMotionInput{.look = {.yaw = 0.5f}}, 1.0f / 60.0f);
        EXPECT_GT(motion.sway.x, 0.0f);

        RunFrames(motion, settings, ViewModelMotionInput{.look = {.yaw = 0.5f}}, 120, 1.0f / 60.0f);
        EXPECT_NEAR(motion.sway.x, 0.0f, 1e-4f);
    }

    TEST(ViewModelMotion, SwayIsLimited)
    {
        ViewModelMotion motion;
        const ViewModelMotionSettings settings;
        UpdateViewModelMotion(motion, settings, ViewModelMotionInput{}, 1.0f / 60.0f);

        UpdateViewModelMotion(motion, settings, ViewModelMotionInput{.look = {.yaw = 100.0f}}, 1.0f / 60.0f);

        EXPECT_LE(glm::length(motion.sway), settings.swayMaximum);
    }

    TEST(ViewModelMotion, LandingDipsAndSpringsBack)
    {
        ViewModelMotion motion;
        const ViewModelMotionSettings settings;

        // Falling at 8 m/s, then standing on the ground.
        UpdateViewModelMotion(motion, settings, ViewModelMotionInput{.isOnGround = false, .verticalSpeed = -8.0f},
                              1.0f / 60.0f);
        UpdateViewModelMotion(motion, settings, ViewModelMotionInput{}, 1.0f / 60.0f);
        EXPECT_LT(motion.inertiaOffset, 0.0f);

        RunFrames(motion, settings, ViewModelMotionInput{}, 120, 1.0f / 60.0f);
        EXPECT_NEAR(motion.inertiaOffset, 0.0f, 1e-4f);
    }

    TEST(ViewModelMotion, JumpDipsTheWeapon)
    {
        ViewModelMotion motion;
        const ViewModelMotionSettings settings;

        UpdateViewModelMotion(motion, settings, ViewModelMotionInput{.isOnGround = false, .verticalSpeed = 8.4f},
                              1.0f / 60.0f);

        EXPECT_LT(motion.inertiaOffset, 0.0f);
    }

    TEST(ViewModelMotion, RecoilPushesBackTurnsUpAndReturns)
    {
        ViewModelMotion motion;
        const ViewModelMotionSettings settings;

        KickViewModelRecoil(motion, settings);
        RunFrames(motion, settings, ViewModelMotionInput{}, 3, 1.0f / 60.0f);
        EXPECT_GT(motion.recoilBack, 0.0f);
        EXPECT_GT(motion.recoilPitch, 0.0f);

        RunFrames(motion, settings, ViewModelMotionInput{}, 120, 1.0f / 60.0f);
        EXPECT_NEAR(motion.recoilBack, 0.0f, 1e-4f);
        EXPECT_NEAR(motion.recoilPitch, 0.0f, 1e-4f);
    }

    TEST(ViewModelMotion, SameMotionAtAnyFrameRate)
    {
        // A landing seen at 30 and at 240 frames per second: 0.1 s after it the weapon is at the same place. The landing
        // frame itself moves the spring by one frame, so one frame fewer follows it.
        const ViewModelMotionSettings settings;
        const auto landAndWait = [&](float deltaTime)
        {
            ViewModelMotion motion;
            UpdateViewModelMotion(motion, settings, ViewModelMotionInput{.isOnGround = false, .verticalSpeed = -8.0f},
                                  deltaTime);
            UpdateViewModelMotion(motion, settings, ViewModelMotionInput{}, deltaTime);
            RunFrames(motion, settings, ViewModelMotionInput{}, static_cast<int>(std::lround(0.1f / deltaTime)) - 1, deltaTime);
            return motion.inertiaOffset;
        };

        EXPECT_NEAR(landAndWait(1.0f / 30.0f), landAndWait(1.0f / 240.0f), 0.002f);
    }
}
