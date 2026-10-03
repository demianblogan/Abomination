#include "Renderer/SkinningBuffer.h"

#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>

#include <algorithm>

namespace Abomination::Renderer
{
    SkinningBuffer::SkinningBuffer()
        : m_buffer(GLBuffer::CreateDynamic(MaximumJointCount * sizeof(glm::mat4)))
    {}

    void SkinningBuffer::Upload(std::span<const glm::mat4> skinningMatrices)
    {
        const std::size_t count = std::min(skinningMatrices.size(), MaximumJointCount);
        m_buffer.Update(std::as_bytes(skinningMatrices.first(count)));

        // A shader storage buffer is not part of the vertex array: it is connected to a numbered binding point, which
        // the shader names with layout(binding = N).
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, JointMatricesStorageBinding, m_buffer.GetID());
    }
}
