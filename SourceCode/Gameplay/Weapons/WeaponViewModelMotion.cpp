#include "Gameplay/Weapons/WeaponViewModelMotion.h"

#include "Core/Math/Spring.h"

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

        // Landings slower than this (meters per second) do not move the weapon: walking down a stair is a small fall.
        constexpr float MinimumLandingSpeed = 2.0f;

        void UpdateBob(WeaponViewModelMotion& motion, const WeaponViewModelMotionSettings& settings,
                       const WeaponViewModelMotionInput& input, float deltaTime)
        {
            // One figure eight (2 pi radians) per stride: the swing follows the distance walked, so it keeps step with
            // the feet at any speed.
            motion.bobPhase += input.horizontalSpeed * deltaTime * 2.0f * std::numbers::pi_v<float> / settings.bobStrideLength;
            motion.bobPhase = std::fmod(motion.bobPhase, 2.0f * std::numbers::pi_v<float>);

            const float targetWeight = input.isOnGround ? glm::clamp(input.horizontalSpeed / input.maxSpeed, 0.0f, 1.0f) : 0.0f;
            motion.bobWeight += (targetWeight - motion.bobWeight) * Core::CalculateApproachFactor(BobFadeRate, deltaTime);

            // The breathing goes on all the time and repeats every two breaths (see CalculateWeaponViewModelMotionOffset);
            // keeping the time within that span keeps it a small number, which a float holds precisely.
            motion.idleTime = std::fmod(motion.idleTime + deltaTime, 2.0f * settings.idleBreathDuration);
        }

        void UpdateSway(WeaponViewModelMotion& motion, const WeaponViewModelMotionSettings& settings,
                        const WeaponViewModelMotionInput& input, float deltaTime)
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
            motion.sway -= motion.sway * Core::CalculateApproachFactor(settings.swayReturnRate, deltaTime);
        }

        void UpdateInertia(WeaponViewModelMotion& motion, const WeaponViewModelMotionSettings& settings,
                           const WeaponViewModelMotionInput& input, float deltaTime)
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

            Core::UpdateDampedSpring(motion.inertiaOffset, motion.inertiaVelocity, settings.springStiffness, deltaTime);
        }
    }

    void UpdateWeaponViewModelMotion(WeaponViewModelMotion& motion, const WeaponViewModelMotionSettings& settings,
                                     const WeaponViewModelMotionInput& input, float deltaTime)
    {
        UpdateBob(motion, settings, input, deltaTime);
        UpdateSway(motion, settings, input, deltaTime);
        UpdateInertia(motion, settings, input, deltaTime);
        Core::UpdateDampedSpring(motion.recoilBack, motion.recoilBackVelocity, settings.recoilStiffness, deltaTime);
        Core::UpdateDampedSpring(motion.recoilPitch, motion.recoilPitchVelocity, settings.recoilStiffness, deltaTime);
    }

    void KickWeaponViewModelRecoil(WeaponViewModelMotion& motion, const WeaponViewModelMotionSettings& settings)
    {
        motion.recoilBackVelocity += settings.recoilKickBack;
        motion.recoilPitchVelocity += settings.recoilKickUp;
    }

    glm::vec3 CalculateWeaponViewModelMotionOffset(const WeaponViewModelMotion& motion,
                                                   const WeaponViewModelMotionSettings& settings)
    {
        // A figure eight lying on its side: one swing left and right (sin of the phase) per stride, and one dip per step,
        // twice as often (sin of twice the phase), smaller.
        const float bob = settings.bobAmount * motion.bobWeight;
        const glm::vec3 bobOffset(std::sin(motion.bobPhase) * bob, std::sin(2.0f * motion.bobPhase) * bob * 0.5f, 0.0f);

        // The breath: up and down once per breath, and to the sides once per two breaths, so the two never line up into
        // a mechanical circle. Strong while standing, gone at full speed, where the bob takes over.
        const float breathAngle = 2.0f * std::numbers::pi_v<float> * motion.idleTime / settings.idleBreathDuration;
        const float idle = settings.idleAmount * (1.0f - motion.bobWeight);
        const glm::vec3 idleOffset(std::sin(breathAngle * 0.5f) * idle * 0.6f, std::sin(breathAngle) * idle, 0.0f);

        return idleOffset + bobOffset + glm::vec3(motion.sway, 0.0f) +
               glm::vec3(0.0f, motion.inertiaOffset, motion.recoilBack);
    }
}
