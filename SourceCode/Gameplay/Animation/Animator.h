#pragma once

#include "Renderer/Animation/SkeletonPose.h"
#include "Renderer/Assets/ModelStore.h"
#include "Renderer/Debug/DebugLines.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Abomination::Gameplay
{
    // A piece of animation the game plays by name ("Idle", "Walk"): a part of one clip of the model, from start to end
    // (seconds). A model may come with one long clip holding all its animations one after another,
    // so a segment names a part of it. end below 0 means the end of the clip.
    struct AnimationSegment
    {
        std::string name;
        std::size_t clip = 0;
        float start = 0.0f;
        float end = -1.0f;

        // Starts again from its beginning when it ends (idle, walk), or stops on its last pose (death).
        bool isLooping = true;
    };

    // Where one segment is being played: which segment and how far into it (seconds from its start).
    struct AnimationPlayback
    {
        std::size_t segment = 0;
        float time = 0.0f;
    };

    // Component: plays the animations of the entity's model and sets its Renderer::ModelPose every frame (see
    // UpdateAnimators). A new segment does not cut in: the old one keeps playing and fades out while the new one fades in
    // (a cross-fade), so the character never jumps from one pose to another.
    struct Animator
    {
        Renderer::ModelHandle model;

        // The pieces the game plays; with none, every clip of the model is one looping segment (see
        // CreateClipSegments).
        std::vector<AnimationSegment> segments;

        // The segment playing now, and the one fading out while it fades in.
        AnimationPlayback current;
        std::optional<AnimationPlayback> previous;

        // How far the cross-fade has come (seconds) and how long it takes.
        float blendTime = 0.0f;
        float blendDuration = 0.0f;

        // How fast the animation plays: 1 as made, 0.5 at half the speed. 0 stops it.
        float speed = 1.0f;

        // Debug: the bones are drawn as lines over everything (see AddAnimatorDebugLines).
        bool isSkeletonVisible = false;

        // Debug: the axes of the model are drawn at its middle: its right (+X, red), up (+Y, green) and the direction the
        // game takes as its front (-Z, blue; see Core::LocalForward).
        bool areAxesVisible = false;

        // Debug: turns and moves the parts held by joints (see Renderer::ModelPose::heldPartAdjustment): meters along the
        // joint's X, Y and Z, and angles around them (radians, applied X first, then Y, then Z).
        glm::vec3 heldPartOffset{0.0f};
        glm::vec3 heldPartAngles{0.0f};
    };

    // One looping segment for every clip of the model, named like the clip, from its start to its end.
    [[nodiscard]] std::vector<AnimationSegment> CreateClipSegments(const Renderer::Model& model);

    // The length of the segment in seconds (its clip's duration for end below 0), never below 0.
    [[nodiscard]] float CalculateSegmentDuration(const AnimationSegment& segment, const Renderer::Model& model);

    // Starts playing the segment from its beginning, cross-fading from what plays now over blendDuration seconds (0:
    // at once). Playing the segment that already plays does nothing, so it can be called every frame.
    void PlayAnimation(Animator& animator, std::size_t segment, float blendDuration);

    // The index of the segment named name, if the animator has one.
    [[nodiscard]] std::optional<std::size_t> FindSegment(const Animator& animator, std::string_view name);

    // The same as PlayAnimation, by the name of the segment; a name the animator does not have plays nothing new.
    void PlayAnimation(Animator& animator, std::string_view name, float blendDuration);

    // Moves the animator on by deltaTime seconds (times the speed): the segments advance, a looping one wraps around,
    // a finished one stays on its last pose, and the cross-fade advances; once it is complete, the old segment is dropped.
    void AdvanceAnimator(Animator& animator, const Renderer::Model& model, float deltaTime);

    // The pose of the skeleton now: the current segment sampled at its time, mixed with the old one during a
    // cross-fade. pose gets one entry per joint; fadingPose is room for the pose of the old segment. Both keep their
    // memory between calls, so posing every animator every frame allocates nothing.
    void CalculateAnimatorPose(const Animator& animator, const Renderer::Model& model, std::vector<Renderer::JointPose>& pose,
                               std::vector<Renderer::JointPose>& fadingPose);

    // Once per frame: advances every animator and sets the Renderer::ModelPose of its entity (the matrix of every joint).
    void UpdateAnimators(entt::registry& registry, const Renderer::ModelStore& models, float deltaTime);

    // The segments the game knows for a model file. For now every clip of the model is one looping segment (see
    // CreateClipSegments); models whose clips need cutting or tuning get their own list here.
    [[nodiscard]] std::vector<AnimationSegment> FindModelSegments(const std::string& path, const Renderer::Model& model);

    // Gives every model of the level that has animation clips (a misc_model of the map) an animator playing its "Idle"
    // segment (or its first one). Called after the level is created.
    void AnimateLevelModels(entt::registry& registry, const Renderer::ModelStore& models);

    // The bones of every animator with isSkeletonVisible, as lines from each joint to its parent (white) and a short
    // axis at every joint, over everything. interpolationFactor places moving entities like the render system does.
    void AddAnimatorDebugLines(const entt::registry& registry, const Renderer::ModelStore& models, float interpolationFactor,
                               Renderer::DebugLines& lines);
}
