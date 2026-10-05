#pragma once

#include "Renderer/Assets/ModelData.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <span>
#include <vector>

namespace Abomination::Renderer
{
    // Where one joint is relative to its parent at one moment: moved, turned and scaled. Kept apart (not as a matrix) so
    // two poses can be blended: positions and scales mix evenly, rotations along the shortest arc.
    struct JointPose
    {
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};
    };

    // The pose of the skeleton when no animation moves it: every joint where the model file puts it.
    [[nodiscard]] std::vector<JointPose> CreateRestPose(const SkeletonData& skeleton);

    // The same into pose, which keeps its memory: no allocation once it is large enough (posing every frame).
    void ResetToRestPose(const SkeletonData& skeleton, std::vector<JointPose>& pose);

    // The transform of every joint in the coordinates of the model (from the model to the joint), from the pose of every
    // joint relative to its parent. The joints are combined from the roots down: a joint's matrix is its parent's matrix
    // times its own transform; a root joint's parent is the root transform of the skeleton. pose has one entry per joint.
    void CalculateJointMatrices(const SkeletonData& skeleton, std::span<const JointPose> pose,
                                std::vector<glm::mat4>& jointMatrices);

    // The matrices a skinned vertex is moved by, one per joint: the joint's matrix now times its inverse bind matrix.
    // A vertex at rest stays where it is (the two cancel out); a vertex of a turned joint turns with it.
    void CalculateSkinningMatrices(const SkeletonData& skeleton, std::span<const glm::mat4> jointMatrices,
                                   std::vector<glm::mat4>& skinningMatrices);
}
