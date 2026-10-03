#include "Gameplay/Animation/Animator.h"

#include <gtest/gtest.h>

#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        // A model of one joint and one clip of 4 s that moves the joint from x = 0 to x = 4 (1 m per second), cut into
        // two segments: "First" (0-2 s, looping) and "Second" (2-4 s, played once).
        Renderer::Model CreateModel()
        {
            Renderer::Model model;
            model.skeleton = Renderer::SkeletonData{.joints = {Renderer::SkeletonJoint{.name = "Root"}}};
            model.animations.push_back(Renderer::AnimationClipData{
                .name = "All",
                .duration = 4.0f,
                .channels = {Renderer::AnimationChannelData{
                    .joint = 0,
                    .path = Renderer::AnimationPath::Translation,
                    .times = {0.0f, 4.0f},
                    .values = {glm::vec4(0.0f), glm::vec4(4.0f, 0.0f, 0.0f, 0.0f)},
                }},
            });
            return model;
        }

        Animator CreateAnimator()
        {
            return Animator{.segments = {
                                AnimationSegment{.name = "First", .start = 0.0f, .end = 2.0f},
                                AnimationSegment{.name = "Second", .start = 2.0f, .end = 4.0f, .isLooping = false},
                            }};
        }

        float CalculateJointX(const Animator& animator, const Renderer::Model& model)
        {
            std::vector<Renderer::JointPose> pose;
            CalculateAnimatorPose(animator, model, pose);
            return pose[0].translation.x;
        }
    }

    TEST(Animator, ClipSegmentsCoverWholeClips)
    {
        const std::vector<AnimationSegment> segments = CreateClipSegments(CreateModel());

        ASSERT_EQ(segments.size(), 1u);
        EXPECT_EQ(segments[0].name, "All");
        EXPECT_FLOAT_EQ(CalculateSegmentDuration(segments[0], CreateModel()), 4.0f);
    }

    TEST(Animator, SegmentPlaysItsPartOfClip)
    {
        const Renderer::Model model = CreateModel();
        Animator animator = CreateAnimator();
        PlayAnimation(animator, 1, 0.0f);

        AdvanceAnimator(animator, model, 0.5f);

        // "Second" starts at 2 s of the clip: half a second in, the joint is at 2.5.
        EXPECT_FLOAT_EQ(CalculateJointX(animator, model), 2.5f);
    }

    TEST(Animator, LoopingSegmentWrapsAround)
    {
        const Renderer::Model model = CreateModel();
        Animator animator = CreateAnimator();

        AdvanceAnimator(animator, model, 2.5f);

        EXPECT_FLOAT_EQ(animator.current.time, 0.5f);
    }

    TEST(Animator, SegmentPlayedOnceStopsOnLastPose)
    {
        const Renderer::Model model = CreateModel();
        Animator animator = CreateAnimator();
        PlayAnimation(animator, 1, 0.0f);

        AdvanceAnimator(animator, model, 10.0f);

        EXPECT_FLOAT_EQ(CalculateJointX(animator, model), 4.0f);
    }

    TEST(Animator, CrossFadeMixesBothSegmentsThenDropsOld)
    {
        const Renderer::Model model = CreateModel();
        Animator animator = CreateAnimator();

        // "First" at its start (x = 0) fades into "Second" over 1 s; speed 0 holds both times still.
        PlayAnimation(animator, 1, 1.0f);
        animator.speed = 0.0f;
        animator.blendTime = 0.5f;

        // Halfway through, the smoothed weight is 0.5: between x = 0 and x = 2.
        EXPECT_FLOAT_EQ(CalculateJointX(animator, model), 1.0f);

        animator.speed = 1.0f;
        AdvanceAnimator(animator, model, 0.6f);
        EXPECT_FALSE(animator.previous.has_value());
    }

    TEST(Animator, PlayingCurrentSegmentAgainChangesNothing)
    {
        const Renderer::Model model = CreateModel();
        Animator animator = CreateAnimator();
        AdvanceAnimator(animator, model, 1.0f);

        PlayAnimation(animator, 0, 0.3f);

        EXPECT_FLOAT_EQ(animator.current.time, 1.0f);
        EXPECT_FALSE(animator.previous.has_value());
    }

    TEST(Animator, SpeedScalesTime)
    {
        const Renderer::Model model = CreateModel();
        Animator animator = CreateAnimator();
        animator.speed = 0.5f;

        AdvanceAnimator(animator, model, 1.0f);

        EXPECT_FLOAT_EQ(animator.current.time, 0.5f);
    }
}
