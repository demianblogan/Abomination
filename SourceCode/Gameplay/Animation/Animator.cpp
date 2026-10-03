#include "Gameplay/Animation/Animator.h"

#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Renderer/Animation/AnimationSampling.h"
#include "Renderer/ModelPose.h"
#include "Renderer/ModelRenderer.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/vec4.hpp>

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

    std::vector<AnimationSegment> FindModelSegments([[maybe_unused]] const std::string& path, const Renderer::Model& model)
    {
        return CreateClipSegments(model);
    }

    void AnimateLevelModels(entt::registry& registry, const Renderer::ModelStore& models)
    {
        for (const auto [entity, modelRenderer] : registry.view<const Renderer::ModelRenderer>(entt::exclude<Animator>).each())
        {
            const Renderer::Model& model = models.Get(modelRenderer.model);
            const std::string* path = models.GetPath(modelRenderer.model);
            if (!model.skeleton.has_value() || model.animations.empty() || path == nullptr)
                continue;

            Animator& animator = registry.emplace<Animator>(entity, Animator{
                .model = modelRenderer.model,
                .segments = FindModelSegments(*path, model),
            });

            // A model stands idle until something else is asked of it; without an "Idle" segment it plays the first one.
            const auto idle = std::ranges::find(animator.segments, "Idle", &AnimationSegment::name);
            if (idle != animator.segments.end())
                animator.current.segment = static_cast<std::size_t>(idle - animator.segments.begin());
        }
    }

    void AddAnimatorDebugLines(const entt::registry& registry, const Renderer::ModelStore& models, float interpolationFactor,
                               Renderer::DebugLines& lines)
    {
        constexpr auto AlwaysVisible = Renderer::DebugLineDepth::AlwaysVisible;
        for (const auto [entity, animator, pose] : registry.view<const Animator, const Renderer::ModelPose>().each())
        {
            const Renderer::Model& model = models.Get(animator.model);

            // The axes of the model at its middle (the origin of the centered model), half a meter long.
            if (animator.areAxesVisible)
            {
                const Core::Transform drawn = Core::CalculateDrawnTransform(registry, entity, interpolationFactor);
                constexpr float AxisLength = 0.5f;
                lines.AddArrow(drawn.position, drawn.position + drawn.rotation * glm::vec3(AxisLength, 0.0f, 0.0f),
                               glm::vec3(1.0f, 0.2f, 0.2f), AlwaysVisible);
                lines.AddArrow(drawn.position, drawn.position + drawn.rotation * glm::vec3(0.0f, AxisLength, 0.0f),
                               glm::vec3(0.2f, 1.0f, 0.2f), AlwaysVisible);
                lines.AddArrow(drawn.position, drawn.position + drawn.rotation * Core::LocalForward * AxisLength,
                               glm::vec3(0.3f, 0.5f, 1.0f), AlwaysVisible);
            }

            if (!animator.isSkeletonVisible || !model.skeleton.has_value() ||
                pose.jointMatrices.size() != model.skeleton->joints.size())
            {
                continue;
            }

            // A joint's matrix takes its origin (0, 0, 0) to where the joint is in the model file; the skeleton
            // transform and the entity take it into the world.
            const glm::mat4 placement =
                Core::CalculateModelMatrix(Core::CalculateDrawnTransform(registry, entity, interpolationFactor)) *
                model.skeletonTransform;
            const auto jointPosition = [&](std::size_t joint)
            {
                return glm::vec3(placement * pose.jointMatrices[joint] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            };

            for (std::size_t joint = 0; joint < model.skeleton->joints.size(); ++joint)
            {
                const glm::vec3 position = jointPosition(joint);
                if (const std::optional<std::size_t> parent = model.skeleton->joints[joint].parent; parent.has_value())
                    lines.AddLine(jointPosition(*parent), position, glm::vec3(1.0f), AlwaysVisible);

                // A small cross at every joint, so the joints at the ends of chains (a toe, the top of the head) show too.
                constexpr float CrossHalfSize = 0.015f;
                lines.AddLine(position - glm::vec3(CrossHalfSize, 0.0f, 0.0f), position + glm::vec3(CrossHalfSize, 0.0f, 0.0f),
                              glm::vec3(1.0f, 0.3f, 0.3f), AlwaysVisible);
                lines.AddLine(position - glm::vec3(0.0f, CrossHalfSize, 0.0f), position + glm::vec3(0.0f, CrossHalfSize, 0.0f),
                              glm::vec3(0.3f, 1.0f, 0.3f), AlwaysVisible);
            }
        }
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

            // Moved along the joint's axes, then turned around them: X first, then Y, then Z (matrices apply right to left).
            const glm::vec3& angles = animator.heldPartAngles;
            modelPose.heldPartAdjustment = glm::translate(glm::mat4(1.0f), animator.heldPartOffset) *
                                           glm::rotate(glm::mat4(1.0f), angles.z, glm::vec3(0.0f, 0.0f, 1.0f)) *
                                           glm::rotate(glm::mat4(1.0f), angles.y, glm::vec3(0.0f, 1.0f, 0.0f)) *
                                           glm::rotate(glm::mat4(1.0f), angles.x, glm::vec3(1.0f, 0.0f, 0.0f));
        }
    }
}
