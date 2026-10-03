#pragma once

#include "Renderer/Animation/SkeletonPose.h"
#include "Renderer/Assets/ModelData.h"

#include <glm/vec4.hpp>

#include <span>

// Reading poses out of animation clips, and mixing two poses: what an animation player does every frame.
namespace Abomination::Renderer
{
    // The value of the channel at time (seconds): between the two keys around it, as the channel's interpolation says.
    // Before the first key the channel holds the first value, after the last key the last one. A rotation (x, y, z, w) is
    // interpolated along the shorter of the two arcs between the keys and comes out of length 1.
    [[nodiscard]] glm::vec4 SampleAnimationChannel(const AnimationChannelData& channel, float time);

    // Sets the joints the clip moves to where they are at time; the joints it does not move keep what pose has.
    // pose has one entry per joint of the skeleton the clip belongs to.
    void SampleAnimationClip(const AnimationClipData& clip, float time, std::span<JointPose> pose);

    // Mixes two poses joint by joint into result: weight 0 gives from, 1 gives to, 0.5 the middle. Places and scales are
    // mixed evenly; rotations along the shorter arc (spherical interpolation), so a joint turns, not shrinks, on the way.
    // All three spans have the same length; result may be the same span as from or to.
    void BlendPoses(std::span<const JointPose> from, std::span<const JointPose> to, float weight,
                    std::span<JointPose> result);
}
