#pragma once

#include "Renderer/Assets/MeshData.h"
#include "Renderer/OpenGL/GLBuffer.h"
#include "Renderer/OpenGL/GLVertexArray.h"

#include <cstddef>
#include <cstdint>

namespace Abomination::Renderer
{
    // Geometry in video memory, ready to be drawn: the vertex buffer, the index buffer and the vertex array that tells
    // OpenGL how to read them (the layout of MeshVertex). Many objects can draw the same mesh: 50 crates are 50 draws of
    // one mesh, the geometry is stored once. Kept in MeshStore and referred to by MeshHandle. Move-only.
    //
    // Every shader that draws meshes reads the position at layout(location = 0), the texture coordinates at
    // layout(location = 1) and the normal at layout(location = 2) (a shader may leave out what it does not need).
    class Mesh
    {
    public:
        // Uploads the vertices and indices to the GPU. data must have at least one triangle.
        [[nodiscard]] static Mesh Create(const MeshData& data);

        // Draws all triangles of the mesh with the shader program and textures that are currently bound.
        void Draw() const;

        [[nodiscard]] std::size_t GetVertexCount() const noexcept;
        [[nodiscard]] std::size_t GetIndexCount() const noexcept;

        // Bytes of video memory taken by both buffers.
        [[nodiscard]] std::size_t GetVideoMemorySize() const noexcept;

    private:
        Mesh(GLBuffer vertexBuffer, GLBuffer indexBuffer, GLVertexArray vertexArray, std::size_t vertexCount,
             std::size_t indexCount) noexcept;

        // The vertex array only refers to the buffers, so it is declared after them and destroyed before them.
        GLBuffer m_vertexBuffer;
        GLBuffer m_indexBuffer;
        GLVertexArray m_vertexArray;

        std::size_t m_vertexCount = 0;
        std::size_t m_indexCount = 0;
    };
}
