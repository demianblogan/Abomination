#include "Gameplay/Animation/Animator.h"

#include "Renderer/Animation/AnimationSampling.h"
#include "Renderer/ModelPose.h"

#include <algorithm>
#include <cmath>

namespace Abomination::Gameplay
{
    namespace
    {
        // The segment, or nullptr if the animator has no such segment or its clip is missing from the model.
        const AnimationSegment* FindSegment(const Animator& animator, const Renderer::Model& model, std::size_t index)
        {
            if (index >= animator.segments.size() || animator.segments[index].clip >= model.animations.size())
                return nullptr;
            return &animator.segments[index];
        }

        // One playback moved on by deltaTime: a looping segment wraps around its length (fmod keeps what is left past
        // the end, so a long frame does not lose time), a finished one holds its end.
        void AdvancePlayback(AnimationPlayback& playback, const Animator& animator, const Renderer::Model& model,
                             float deltaTime)
        {
            const AnimationSegment* segment = FindSegment(animator, model, playback.segment);
            if (segment == nullptr)
                return;

            const float duration = CalculateSegmentDuration(*segment, model);
            playback.time += deltaTime;
            if (duration <= 0.0f)
                playback.time = 0.0f;
            else if (segment->isLooping)
                playback.time = std::fmod(playback.time, duration);
            else
                playback.time = std::min(playback.time, duration);
        }

        // The pose of one playback: the rest pose with the joints its clip moves set to the segment's time in the clip.
        void SamplePlayback(const AnimationPlayback& playback, const Animator& animator, const Renderer::Model& model,
                            std::vector<Renderer::JointPose>& pose)
        {
            pose = Renderer::CreateRestPose(*model.skeleton);
            if (const AnimationSegment* segment = FindSegment(animator, model, playback.segment); segment != nullptr)
                Renderer::SampleAnimationClip(model.animations[segment->clip], segment->start + playback.time, pose);
        }
    }

    std::vector<AnimationSegment> CreateClipSegments(const Renderer::Model& model)
    {
        std::vector<AnimationSegment> segments;
        for (std::size_t clip = 0; clip < model.animations.size(); ++clip)
            segments.push_back({.name = model.animations[clip].name, .clip = clip});
        return segments;
    }

    float CalculateSegmentDuration(const AnimationSegment& segment, const Renderer::Model& model)
    {
        if (segment.clip >= model.animations.size())
            return 0.0f;

        const float end = segment.end < 0.0f ? model.animations[segment.clip].duration : segment.end;
        return std::max(end - segment.start, 0.0f);
    }

    void PlayAnimation(Animator& animator, std::size_t segment, float blendDuration)
    {
        if (segment == animator.current.segment)
            return;

        // What plays now fades out from where it is; the new segment starts from its beginning.
        animator.previous = blendDuration > 0.0f ? std::optional(animator.current) : std::nullopt;
        animator.current = AnimationPlayback{.segment = segment};
        animator.blendTime = 0.0f;
        animator.blendDuration = std::max(blendDuration, 0.0f);
    }

    void AdvanceAnimator(Animator& animator, const Renderer::Model& model, float deltaTime)
    {
        const float scaledTime = deltaTime * animator.speed;
        AdvancePlayback(animator.current, animator, model, scaledTime);
        if (!animator.previous.has_value())
            return;

        AdvancePlayback(*animator.previous, animator, model, scaledTime);
        animator.blendTime += scaledTime;
        if (animator.blendTime >= animator.blendDuration)
            animator.previous.reset();
    }

    void CalculateAnimatorPose(const Animator& animator, const Renderer::Model& model, std::vector<Renderer::JointPose>& pose)
    {
        if (!model.skeleton.has_value())
        {
            pose.clear();
            return;
        }

        SamplePlayback(animator.current, animator, model, pose);
        if (!animator.previous.has_value() || animator.blendDuration <= 0.0f)
            return;

        // The new segment's share grows from 0 to 1 over the cross-fade, smoothly (3t^2 - 2t^3): it starts and finishes
        // gently instead of switching at a constant rate.
        std::vector<Renderer::JointPose> previousPose;
        SamplePlayback(*animator.previous, animator, model, previousPose);
        const float t = std::clamp(animator.blendTime / animator.blendDuration, 0.0f, 1.0f);
        const float weight = t * t * (3.0f - 2.0f * t);
        Renderer::BlendPoses(previousPose, pose, weight, pose);
    }

    void UpdateAnimators(entt::registry& registry, const Renderer::ModelStore& models, float deltaTime)
    {
        std::vector<Renderer::JointPose> pose;
        for (const auto [entity, animator] : registry.view<Animator>().each())
        {
            const Renderer::Model& model = models.Get(animator.model);
            if (!model.skeleton.has_value())
                continue;

            AdvanceAnimator(animator, model, deltaTime);
            CalculateAnimatorPose(animator, model, pose);

            Renderer::ModelPose& modelPose = registry.get_or_emplace<Renderer::ModelPose>(entity);
            Renderer::CalculateJointMatrices(*model.skeleton, pose, modelPose.jointMatrices);
        }
    }
}
