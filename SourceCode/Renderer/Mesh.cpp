#include "Renderer/Mesh.h"

#include "Renderer/ShaderInterface.h"

#include <glad/gl.h>

#include <cassert>
#include <span>
#include <utility>

namespace Abomination::Renderer
{
    namespace
    {
        // The vertex array has only one vertex buffer, connected to binding slot 0.
        constexpr std::uint32_t VertexBufferBinding = 0;
    }

    Mesh Mesh::Create(const MeshData& data)
    {
        // At least one triangle, and only whole triangles: every 3 indices are one. MeshStore replaces empty data with the
        // fallback cube before it gets here.
        assert(!data.vertices.empty() && data.indices.size() >= 3 && data.indices.size() % 3 == 0);

        // Upload the vertices and the indices to the GPU once; from now on they live in video memory.
        GLBuffer vertexBuffer(std::as_bytes(std::span(data.vertices)));
        GLBuffer indexBuffer(std::as_bytes(std::span(data.indices)));

        // Describe the layout of MeshVertex and connect both buffers.
        GLVertexArray vertexArray;
        vertexArray.SetVertexBuffer(VertexBufferBinding, vertexBuffer, sizeof(MeshVertex));
        vertexArray.SetFloatAttribute(MeshPositionAttribute, VertexBufferBinding, 3, offsetof(MeshVertex, position));
        vertexArray.SetFloatAttribute(MeshTexCoordAttribute, VertexBufferBinding, 2, offsetof(MeshVertex, texCoord));
        vertexArray.SetFloatAttribute(MeshNormalAttribute, VertexBufferBinding, 3, offsetof(MeshVertex, normal));
        vertexArray.SetIndexBuffer(indexBuffer);

        return Mesh(std::move(vertexBuffer), std::move(indexBuffer), std::move(vertexArray), data.vertices.size(),
                    data.indices.size());
    }

    Mesh::Mesh(GLBuffer vertexBuffer, GLBuffer indexBuffer, GLVertexArray vertexArray, std::size_t vertexCount,
               std::size_t indexCount) noexcept
        : m_vertexBuffer(std::move(vertexBuffer))
        , m_indexBuffer(std::move(indexBuffer))
        , m_vertexArray(std::move(vertexArray))
        , m_vertexCount(vertexCount)
        , m_indexCount(indexCount)
    {}

    void Mesh::Draw() const
    {
        // glDrawElements takes m_indexCount indices from the index buffer of the bound vertex array and draws a triangle
        // for every 3 of them. nullptr is the offset into the index buffer: start at the first index.
        m_vertexArray.Bind();
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    }

    std::size_t Mesh::GetVertexCount() const noexcept
    {
        return m_vertexCount;
    }

    std::size_t Mesh::GetIndexCount() const noexcept
    {
        return m_indexCount;
    }

    std::size_t Mesh::GetVideoMemorySize() const noexcept
    {
        return m_vertexCount * sizeof(MeshVertex) + m_indexCount * sizeof(std::uint32_t);
    }
}
