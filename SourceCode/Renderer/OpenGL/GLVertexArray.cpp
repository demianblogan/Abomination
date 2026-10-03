#include "Renderer/OpenGL/GLVertexArray.h"

#include "Renderer/OpenGL/GLBuffer.h"

#include <glad/gl.h>

#include <utility>

namespace Abomination::Renderer
{
    GLVertexArray::GLVertexArray()
    {
        // Direct State Access: created by ID, configured below without binding (glGenVertexArrays + glBindVertexArray
        // in the old way).
        glCreateVertexArrays(1, &m_vertexArrayID);
    }

    GLVertexArray::GLVertexArray(GLVertexArray&& other) noexcept
        : m_vertexArrayID(std::exchange(other.m_vertexArrayID, 0))
    {}

    GLVertexArray& GLVertexArray::operator=(GLVertexArray&& other) noexcept
    {
        if (this != &other)
        {
            // glDelete* functions ignore the ID 0, so a moved-from object needs no special check.
            glDeleteVertexArrays(1, &m_vertexArrayID);
            m_vertexArrayID = std::exchange(other.m_vertexArrayID, 0);
        }

        return *this;
    }

    GLVertexArray::~GLVertexArray()
    {
        glDeleteVertexArrays(1, &m_vertexArrayID);
    }

    void GLVertexArray::SetVertexBuffer(std::uint32_t bindingIndex, const GLBuffer& buffer, std::size_t stride)
    {
        // Binding slot bindingIndex reads vertices from the buffer, starting at byte 0, one vertex every "stride" bytes.
        glVertexArrayVertexBuffer(m_vertexArrayID, bindingIndex, buffer.GetID(), 0, static_cast<GLsizei>(stride));
    }

    void GLVertexArray::SetFloatAttribute(std::uint32_t attributeIndex, std::uint32_t bindingIndex, int componentCount,
                                          std::size_t offset)
    {
        // The old glVertexAttribPointer did all three steps at once and took the buffer from the global binding point.
        // With DSA the format of the attribute and the buffer it comes from are set separately.

        // 1. The attribute is read from the buffer at all (disabled attributes get a constant value instead).
        glEnableVertexArrayAttrib(m_vertexArrayID, attributeIndex);

        // 2. Its format: componentCount floats, not normalized, starting "offset" bytes from the start of a vertex.
        glVertexArrayAttribFormat(m_vertexArrayID, attributeIndex, componentCount, GL_FLOAT, GL_FALSE,
                                  static_cast<GLuint>(offset));

        // 3. Which binding slot (and therefore which buffer) it is read from.
        glVertexArrayAttribBinding(m_vertexArrayID, attributeIndex, bindingIndex);
    }

    void GLVertexArray::SetNormalizedByteAttribute(std::uint32_t attributeIndex, std::uint32_t bindingIndex,
                                                   int componentCount, std::size_t offset)
    {
        // The same three steps as SetFloatAttribute; GL_TRUE ("normalized") makes OpenGL divide every byte by 255.
        glEnableVertexArrayAttrib(m_vertexArrayID, attributeIndex);
        glVertexArrayAttribFormat(m_vertexArrayID, attributeIndex, componentCount, GL_UNSIGNED_BYTE, GL_TRUE,
                                  static_cast<GLuint>(offset));
        glVertexArrayAttribBinding(m_vertexArrayID, attributeIndex, bindingIndex);
    }

    void GLVertexArray::SetUnsignedIntAttribute(std::uint32_t attributeIndex, std::uint32_t bindingIndex,
                                                int componentCount, std::size_t offset)
    {
        // The same three steps; glVertexArrayAttribIFormat ("I": integer) keeps the numbers integers. The float version
        // would turn joint 3 into 3.0, which a shader cannot use to index an array.
        glEnableVertexArrayAttrib(m_vertexArrayID, attributeIndex);
        glVertexArrayAttribIFormat(m_vertexArrayID, attributeIndex, componentCount, GL_UNSIGNED_INT,
                                   static_cast<GLuint>(offset));
        glVertexArrayAttribBinding(m_vertexArrayID, attributeIndex, bindingIndex);
    }

    void GLVertexArray::SetIndexBuffer(const GLBuffer& buffer)
    {
        // The old way: glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id) while the vertex array is bound.
        glVertexArrayElementBuffer(m_vertexArrayID, buffer.GetID());
    }

    void GLVertexArray::Bind() const
    {
        glBindVertexArray(m_vertexArrayID);
    }
}
