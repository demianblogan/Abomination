#include "Renderer/Animation/AnimationSampling.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <gtest/gtest.h>

#include <numbers>
#include <vector>

namespace Abomination::Renderer
{
    namespace
    {
        // A translation channel of joint 0: at 0 s at the origin, at 2 s 4 m along X.
        AnimationChannelData CreateMove(AnimationInterpolation interpolation)
        {
            return AnimationChannelData{
                .joint = 0,
                .path = AnimationPath::Translation,
                .interpolation = interpolation,
                .times = {0.0f, 2.0f},
                .values = {glm::vec4(0.0f), glm::vec4(4.0f, 0.0f, 0.0f, 0.0f)},
            };
        }

        // A rotation as glTF stores it: (x, y, z, w).
        glm::vec4 RotationAroundZ(float angle)
        {
            const glm::quat rotation = glm::angleAxis(angle, glm::vec3(0.0f, 0.0f, 1.0f));
            return {rotation.x, rotation.y, rotation.z, rotation.w};
        }
    }

    TEST(AnimationSampling, LinearChannelMixesBetweenKeys)
    {
        const glm::vec4 value = SampleAnimationChannel(CreateMove(AnimationInterpolation::Linear), 0.5f);

        EXPECT_FLOAT_EQ(value.x, 1.0f);
    }

    TEST(AnimationSampling, StepChannelHoldsPreviousKey)
    {
        const glm::vec4 value = SampleAnimationChannel(CreateMove(AnimationInterpolation::Step), 1.9f);

        EXPECT_FLOAT_EQ(value.x, 0.0f);
    }

    TEST(AnimationSampling, ChannelHoldsEndsOutsideItsKeys)
    {
        const AnimationChannelData channel = CreateMove(AnimationInterpolation::Linear);

        EXPECT_FLOAT_EQ(SampleAnimationChannel(channel, -1.0f).x, 0.0f);
        EXPECT_FLOAT_EQ(SampleAnimationChannel(channel, 5.0f).x, 4.0f);
    }

    TEST(AnimationSampling, RotationTakesShorterArc)
    {
        // From +170 degrees to -170 degrees around Z: the short way is 20 degrees through 180, not 340 through 0. The
        // second key is stored negated (the same rotation), which a naive interpolation would turn into the long way.
        constexpr float Degree = std::numbers::pi_v<float> / 180.0f;
        AnimationChannelData channel{
            .joint = 0,
            .path = AnimationPath::Rotation,
            .times = {0.0f, 1.0f},
            .values = {RotationAroundZ(170.0f * Degree), -RotationAroundZ(-170.0f * Degree)},
        };

        const glm::vec4 value = SampleAnimationChannel(channel, 0.5f);

        // Halfway is 180 degrees: the rotation turns +X into -X.
        const glm::vec3 turned = glm::quat(value.w, value.x, value.y, value.z) * glm::vec3(1.0f, 0.0f, 0.0f);
        EXPECT_NEAR(turned.x, -1.0f, 1e-5f);
        EXPECT_NEAR(glm::length(value), 1.0f, 1e-5f);
    }

    TEST(AnimationSampling, ClipSetsOnlyJointsItMoves)
    {
        AnimationClipData clip{.name = "Move", .duration = 2.0f, .channels = {CreateMove(AnimationInterpolation::Linear)}};
        std::vector<JointPose> pose(2);
        pose[1].translation = glm::vec3(0.0f, 7.0f, 0.0f);

        SampleAnimationClip(clip, 1.0f, pose);

        EXPECT_FLOAT_EQ(pose[0].translation.x, 2.0f);
        EXPECT_FLOAT_EQ(pose[1].translation.y, 7.0f);
    }

    TEST(AnimationSampling, BlendMixesPlacesAndRotations)
    {
        std::vector<JointPose> from(1);
        std::vector<JointPose> to(1);
        to[0].translation = glm::vec3(2.0f, 0.0f, 0.0f);
        to[0].rotation = glm::angleAxis(std::numbers::pi_v<float> / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        std::vector<JointPose> result(1);

        BlendPoses(from, to, 0.5f, result);

        EXPECT_FLOAT_EQ(result[0].translation.x, 1.0f);

        // Half of 90 degrees: +X turns to (cos 45, sin 45).
        const glm::vec3 turned = result[0].rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        EXPECT_NEAR(turned.x, std::numbers::sqrt2_v<float> / 2.0f, 1e-5f);
        EXPECT_NEAR(turned.y, std::numbers::sqrt2_v<float> / 2.0f, 1e-5f);
    }
}
