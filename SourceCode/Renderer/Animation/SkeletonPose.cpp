#include "Renderer/Animation/SkeletonPose.h"

#include "Core/Profiling/ProfileZone.h"

#include <glm/ext/matrix_transform.hpp>

#include <cassert>
#include <cstddef>

namespace Abomination::Renderer
{
    std::vector<JointPose> CreateRestPose(const SkeletonData& skeleton)
    {
        std::vector<JointPose> pose;
        ResetToRestPose(skeleton, pose);
        return pose;
    }

    void ResetToRestPose(const SkeletonData& skeleton, std::vector<JointPose>& pose)
    {
        pose.resize(skeleton.joints.size());
        for (std::size_t index = 0; index < skeleton.joints.size(); ++index)
        {
            const SkeletonJoint& joint = skeleton.joints[index];
            pose[index] = {.translation = joint.translation, .rotation = joint.rotation, .scale = joint.scale};
        }
    }

    void CalculateJointMatrices(const SkeletonData& skeleton, std::span<const JointPose> pose,
                                std::vector<glm::mat4>& jointMatrices)
    {
        PROFILE_ZONE();

        assert(pose.size() == skeleton.joints.size());
        jointMatrices.resize(skeleton.joints.size());
        for (std::size_t index = 0; index < skeleton.joints.size(); ++index)
        {
            // The joint's own transform, like a Core::Transform: scaled first, then turned, then moved (T * R * S).
            const JointPose& joint = pose[index];
            const glm::mat4 local = glm::translate(glm::mat4(1.0f), joint.translation) * glm::mat4_cast(joint.rotation) *
                                    glm::scale(glm::mat4(1.0f), joint.scale);

            // The parents come first in the skeleton (see SkeletonData), so the parent's matrix is ready.
            const std::optional<std::size_t> parent = skeleton.joints[index].parent;
            jointMatrices[index] = (parent.has_value() ? jointMatrices[*parent] : skeleton.rootTransform) * local;
        }
    }

    void CalculateSkinningMatrices(const SkeletonData& skeleton, std::span<const glm::mat4> jointMatrices,
                                   std::vector<glm::mat4>& skinningMatrices)
    {
        assert(jointMatrices.size() == skeleton.joints.size());
        skinningMatrices.resize(skeleton.joints.size());
        for (std::size_t index = 0; index < skeleton.joints.size(); ++index)
            skinningMatrices[index] = jointMatrices[index] * skeleton.joints[index].inverseBindMatrix;
    }
}
