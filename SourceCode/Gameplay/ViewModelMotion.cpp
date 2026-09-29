#include "Gameplay/ViewModelMotion.h"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Abomination::Gameplay
{
    namespace
    {
        // How fast the bob fades in when the player starts running and out when they stop or leave the ground (per
        // second, see swayReturnRate).
        constexpr float BobFadeRate = 8.0f;

        // The spring is integrated in steps of at most this long (seconds): a spring moved by a whole slow frame at once
        // (a long frame of 0.1 s) would overshoot and could swing ever wider. 240 steps per second keep it stable.
        constexpr float SpringStepDuration = 1.0f / 240.0f;

        // Landings slower than this (meters per second) do not move the weapon: walking down a stair is a small fall.
        constexpr float MinimumLandingSpeed = 2.0f;

        // The fraction of a gap that is closed during deltaTime when rate of it is closed per second, the same way at any
        // frame rate: after two frames of 0.01 s exactly as much is closed as after one frame of 0.02 s. (Closing
        // rate * deltaTime of it each frame, the simple way, would close less at a low frame rate.)
        float CalculateApproachFactor(float rate, float deltaTime)
        {
            return 1.0f - std::exp(-rate * deltaTime);
        }

        void UpdateBob(ViewModelMotion& motion, const ViewModelMotionSettings& settings, const ViewModelMotionInput& input,
                       float deltaTime)
        {
            // One figure eight (2 pi radians) per stride: the swing follows the distance walked, so it keeps step with
            // the feet at any speed.
            motion.bobPhase += input.horizontalSpeed * deltaTime * 2.0f * std::numbers::pi_v<float> / settings.bobStrideLength;
            motion.bobPhase = std::fmod(motion.bobPhase, 2.0f * std::numbers::pi_v<float>);

            const float targetWeight = input.isOnGround ? glm::clamp(input.horizontalSpeed / input.maxSpeed, 0.0f, 1.0f) : 0.0f;
            motion.bobWeight += (targetWeight - motion.bobWeight) * CalculateApproachFactor(BobFadeRate, deltaTime);
        }

        void UpdateSway(ViewModelMotion& motion, const ViewModelMotionSettings& settings, const ViewModelMotionInput& input,
                        float deltaTime)
        {
            // How much the view turned since the last frame. The very first frame has nothing to compare with.
            if (motion.hasPreviousLook)
            {
                const float yawChange = input.look.yaw - motion.previousLook.yaw;
                const float pitchChange = input.look.pitch - motion.previousLook.pitch;

                // The weapon lags behind: turning left (a bigger yaw) leaves it to the right, looking up leaves it lower.
                motion.sway += glm::vec2(yawChange, -pitchChange) * settings.swayAmount;

                const float length = glm::length(motion.sway);
                if (length > settings.swayMaximum)
                    motion.sway *= settings.swayMaximum / length;
            }
            motion.previousLook = input.look;
            motion.hasPreviousLook = true;

            // And catches up with the view.
            motion.sway -= motion.sway * CalculateApproachFactor(settings.swayReturnRate, deltaTime);
        }

        void UpdateInertia(ViewModelMotion& motion, const ViewModelMotionSettings& settings,
                           const ViewModelMotionInput& input, float deltaTime)
        {
            // A jump: the body shoots up, the weapon stays behind for a moment, so it is pushed down.
            if (motion.wasOnGround && !input.isOnGround && input.verticalSpeed > 0.0f)
                motion.inertiaVelocity -= settings.jumpKick;

            // A landing: the body stops at once, the weapon goes on down for a moment. The fall speed comes from the last
            // frame: by now the physics has stopped the fall.
            const float fallSpeed = -motion.previousVerticalSpeed;
            if (!motion.wasOnGround && input.isOnGround && fallSpeed > MinimumLandingSpeed)
                motion.inertiaVelocity -= fallSpeed * settings.landingKickPerFallSpeed;

            motion.wasOnGround = input.isOnGround;
            motion.previousVerticalSpeed = input.verticalSpeed;

            // A damped spring pulls the weapon back to where it is held: the spring accelerates it towards 0 in
            // proportion to how far away it is (stiffness), the damping brakes it in proportion to its speed. Damping of
            // 1.4 * sqrt(stiffness) is a little below the value at which it would creep back without any swing (2 *
            // sqrt(stiffness)), so it swings past 0 once, slightly, like a real hand catching the weight.
            const float damping = 1.4f * std::sqrt(settings.springStiffness);
            float remainingTime = deltaTime;
            while (remainingTime > 0.0f)
            {
                const float step = std::min(remainingTime, SpringStepDuration);
                const float acceleration = -settings.springStiffness * motion.inertiaOffset - damping * motion.inertiaVelocity;

                // Semi-implicit Euler: the new velocity moves the position. Unlike the plain order, it does not add energy
                // to a spring, so the swing does not grow by itself.
                motion.inertiaVelocity += acceleration * step;
                motion.inertiaOffset += motion.inertiaVelocity * step;
                remainingTime -= step;
            }
        }
    }

    void UpdateViewModelMotion(ViewModelMotion& motion, const ViewModelMotionSettings& settings,
                               const ViewModelMotionInput& input, float deltaTime)
    {
        UpdateBob(motion, settings, input, deltaTime);
        UpdateSway(motion, settings, input, deltaTime);
        UpdateInertia(motion, settings, input, deltaTime);
    }

    glm::vec3 CalculateViewModelMotionOffset(const ViewModelMotion& motion, const ViewModelMotionSettings& settings)
    {
        // A figure eight lying on its side: one swing left and right (sin of the phase) per stride, and one dip per step,
        // twice as often (sin of twice the phase), smaller.
        const float bob = settings.bobAmount * motion.bobWeight;
        const glm::vec3 bobOffset(std::sin(motion.bobPhase) * bob, std::sin(2.0f * motion.bobPhase) * bob * 0.5f, 0.0f);

        return bobOffset + glm::vec3(motion.sway, 0.0f) + glm::vec3(0.0f, motion.inertiaOffset, 0.0f);
    }
}
