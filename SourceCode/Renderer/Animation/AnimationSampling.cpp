#include "Renderer/Animation/AnimationSampling.h"

#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>

namespace Abomination::Renderer
{
    namespace
    {
        // glTF stores a rotation as (x, y, z, w); glm::quat is built as (w, x, y, z).
        glm::quat ToQuaternion(const glm::vec4& value)
        {
            return glm::quat(value.w, value.x, value.y, value.z);
        }

        glm::vec4 ToVector(const glm::quat& rotation)
        {
            return glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w);
        }

        // The shorter arc between two rotations. A quaternion and its negative are the same rotation, but spherical
        // interpolation from q to -q goes the long way around (a full turn). If the two point to opposite halves (their
        // dot product is negative), the second is flipped, which keeps the rotation and makes the arc the shorter one.
        glm::quat SlerpShortest(const glm::quat& from, glm::quat to, float weight)
        {
            if (glm::dot(from, to) < 0.0f)
                to = -to;
            return glm::normalize(glm::slerp(from, to, weight));
        }
    }

    glm::vec4 SampleAnimationChannel(const AnimationChannelData& channel, float time)
    {
        assert(!channel.times.empty() && channel.times.size() == channel.values.size());

        // The first key after time. upper_bound searches the sorted times by halving: a clip of 700 keys takes about 10
        // steps instead of 700.
        const auto next = std::upper_bound(channel.times.begin(), channel.times.end(), time);
        if (next == channel.times.begin())
            return channel.values.front();
        if (next == channel.times.end())
            return channel.values.back();

        const auto nextIndex = static_cast<std::size_t>(std::distance(channel.times.begin(), next));
        const std::size_t previousIndex = nextIndex - 1;
        const glm::vec4& previousValue = channel.values[previousIndex];
        if (channel.interpolation == AnimationInterpolation::Step)
            return previousValue;

        // How far time is between the two keys: 0 at the previous one, 1 at the next one.
        const float previousTime = channel.times[previousIndex];
        const float span = channel.times[nextIndex] - previousTime;
        const float weight = span > 0.0f ? (time - previousTime) / span : 0.0f;
        const glm::vec4& nextValue = channel.values[nextIndex];

        if (channel.path == AnimationPath::Rotation)
            return ToVector(SlerpShortest(ToQuaternion(previousValue), ToQuaternion(nextValue), weight));

        return glm::mix(previousValue, nextValue, weight);
    }

    void SampleAnimationClip(const AnimationClipData& clip, float time, std::span<JointPose> pose)
    {
        for (const AnimationChannelData& channel : clip.channels)
        {
            if (channel.joint >= pose.size() || channel.times.empty())
                continue;

            const glm::vec4 value = SampleAnimationChannel(channel, time);
            JointPose& joint = pose[channel.joint];
            switch (channel.path)
            {
            case AnimationPath::Translation:
                joint.translation = glm::vec3(value);
                break;
            case AnimationPath::Rotation:
                joint.rotation = ToQuaternion(value);
                break;
            case AnimationPath::Scale:
                joint.scale = glm::vec3(value);
                break;
            }
        }
    }

    void BlendPoses(std::span<const JointPose> from, std::span<const JointPose> to, float weight,
                    std::span<JointPose> result)
    {
        assert(from.size() == to.size() && from.size() == result.size());
        for (std::size_t index = 0; index < result.size(); ++index)
        {
            // Copies first: result may be the same memory as from or to.
            const JointPose a = from[index];
            const JointPose& b = to[index];
            result[index] = JointPose{
                .translation = glm::mix(a.translation, b.translation, weight),
                .rotation = SlerpShortest(a.rotation, b.rotation, weight),
                .scale = glm::mix(a.scale, b.scale, weight),
            };
        }
    }
}
