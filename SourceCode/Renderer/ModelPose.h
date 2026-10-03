#pragma once

#include <glm/mat4x4.hpp>

#include <vector>

namespace Abomination::Renderer
{
    // Component: the pose a model with a skeleton is drawn in now, set by whatever animates it (see Gameplay::Animator).
    // jointMatrices has one matrix per joint of the model's skeleton, in the coordinates of the model file (see
    // CalculateJointMatrices). An entity without it, or with the wrong number of matrices, is drawn at rest.
    struct ModelPose
    {
        std::vector<glm::mat4> jointMatrices;
    };
}
