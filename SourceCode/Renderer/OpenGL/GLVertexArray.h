#pragma once

#include <cstddef>
#include <cstdint>

namespace Abomination::Renderer
{
    class GLBuffer;

    // A vertex array object (VAO): describes how the vertex shader gets its input from buffers.
    //   - Buffer bindings: which buffer the vertices come from and how many bytes one vertex takes (stride).
    //   - Attributes: for each shader input (layout(location = N) in ...) its type, the number of components
    //     and where it starts inside one vertex.
    // The VAO does not own the buffers: they must live at least as long as they are drawn with. Move-only.
    class GLVertexArray
    {
    public:
        GLVertexArray();

        GLVertexArray(const GLVertexArray&) = delete;
        GLVertexArray& operator=(const GLVertexArray&) = delete;

        GLVertexArray(GLVertexArray&& other) noexcept;
        GLVertexArray& operator=(GLVertexArray&& other) noexcept;

        ~GLVertexArray();

        // Connects a buffer with vertices to a binding slot. stride is the size of one vertex in bytes.
        void SetVertexBuffer(std::uint32_t bindingIndex, const GLBuffer& buffer, std::size_t stride);

        // Describes one shader input made of float components (float, vec2, vec3, vec4):
        // attributeIndex is its layout(location), componentCount is 1-4, offset is its position inside one vertex
        // in bytes, and bindingIndex is the slot of the buffer it is read from.
        void SetFloatAttribute(std::uint32_t attributeIndex, std::uint32_t bindingIndex, int componentCount,
                               std::size_t offset);

        // Connects an index buffer (element buffer): a list of vertex numbers that tells glDrawElements in which
        // order to take the vertices, so a vertex shared by several triangles is stored only once.
        void SetIndexBuffer(const GLBuffer& buffer);

        // Makes this vertex array the one the next draw calls read vertices through.
        void Bind() const;

    private:
        // 0 means "no vertex array" (a moved-from object).
        std::uint32_t m_vertexArrayID = 0;
    };
}
