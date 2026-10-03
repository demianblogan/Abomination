#pragma once

#include "Renderer/OpenGL/GLBuffer.h"

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <span>

namespace Abomination::Renderer
{
    // The joint matrices of the skinned mesh about to be drawn, in video memory: a shader storage buffer the vertex shader
    // reads (see JointMatricesStorageBinding). One buffer serves every skinned mesh: each draw uploads its matrices just
    // before it. Requires a current OpenGL context. Move-only.
    class SkinningBuffer
    {
    public:
        // More joints than any skeleton of the game has (a character has 30-70).
        static constexpr std::size_t MaximumJointCount = 256;

        SkinningBuffer();

        // Copies the matrices into the buffer and connects it to the binding the shaders read; extra joints beyond
        // MaximumJointCount are left out.
        void Upload(std::span<const glm::mat4> skinningMatrices);

    private:
        GLBuffer m_buffer;
    };
}
