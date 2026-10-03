#include "Renderer/Assets/Mesh.h"

#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>

#include <cassert>
#include <span>
#include <utility>

namespace Abomination::Renderer
{
    namespace
    {
        // The vertices are read from binding slot 0; the skin of a skinned mesh from slot 1.
        constexpr std::uint32_t VertexBufferBinding = 0;
        constexpr std::uint32_t SkinBufferBinding = 1;
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

        // A skinned mesh: the joints and weights of every vertex in a second buffer, so a rigid mesh does not carry them.
        std::optional<GLBuffer> skinBuffer;
        if (!data.skin.empty())
        {
            assert(data.skin.size() == data.vertices.size());
            skinBuffer.emplace(std::as_bytes(std::span(data.skin)));
            vertexArray.SetVertexBuffer(SkinBufferBinding, *skinBuffer, sizeof(VertexSkin));
            vertexArray.SetUnsignedIntAttribute(MeshJointsAttribute, SkinBufferBinding, 4, offsetof(VertexSkin, joints));
            vertexArray.SetFloatAttribute(MeshWeightsAttribute, SkinBufferBinding, 4, offsetof(VertexSkin, weights));
        }

        return Mesh(std::move(vertexBuffer), std::move(indexBuffer), std::move(skinBuffer), std::move(vertexArray),
                    data.vertices.size(), data.indices.size());
    }

    Mesh::Mesh(GLBuffer vertexBuffer, GLBuffer indexBuffer, std::optional<GLBuffer> skinBuffer, GLVertexArray vertexArray,
               std::size_t vertexCount, std::size_t indexCount) noexcept
        : m_vertexBuffer(std::move(vertexBuffer))
        , m_indexBuffer(std::move(indexBuffer))
        , m_skinBuffer(std::move(skinBuffer))
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

    bool Mesh::IsSkinned() const noexcept
    {
        return m_skinBuffer.has_value();
    }

    std::size_t Mesh::GetVideoMemorySize() const noexcept
    {
        const std::size_t skinSize = IsSkinned() ? m_vertexCount * sizeof(VertexSkin) : 0;
        return m_vertexCount * sizeof(MeshVertex) + m_indexCount * sizeof(std::uint32_t) + skinSize;
    }
}
