#pragma once

#include "Renderer/Assets/ModelData.h"
#include "Renderer/OpenGL/GLBuffer.h"

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace Abomination::Renderer
{
    // The skinning matrices of the model about to be drawn, in video memory: a shader storage buffer the vertex shader
    // reads (see JointMatricesStorageBinding). One buffer serves every skinned model: each model uploads its pose once,
    // just before its parts are drawn. Requires a current OpenGL context. Move-only.
    class SkinningBuffer
    {
    public:
        // More joints than any skeleton of the game has (a character has 30-70).
        static constexpr std::size_t MaximumJointCount = 256;

        SkinningBuffer();

        // Calculates the skinning matrices of the skeleton in the pose jointMatrices (see CalculateSkinningMatrices),
        // copies them into the buffer and connects it to the binding the shaders read; extra joints beyond
        // MaximumJointCount are left out. The matrices are worked out in memory the buffer keeps, so drawing every
        // skinned model every frame allocates nothing.
        void UploadPose(const SkeletonData& skeleton, std::span<const glm::mat4> jointMatrices);

    private:
        GLBuffer m_buffer;
        std::vector<glm::mat4> m_skinningMatrices;
    };
}
