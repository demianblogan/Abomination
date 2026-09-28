#include "Renderer/Debug/DebugLineRenderer.h"

#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>

#include <array>
#include <cstdint>

namespace Abomination::Renderer
{
    namespace
    {
        // The vertex array has only one vertex buffer, connected to binding slot 0.
        constexpr std::uint32_t VertexBufferBinding = 0;

        // Every line is a strip of 2 triangles: 6 corners.
        constexpr std::size_t VerticesPerLine = 6;

        // Enough for about 700 lines; a frame with more makes the buffer grow.
        constexpr std::size_t InitialVertexCapacity = 4096;

        // The corners of the two triangles of a strip: (0, -1) is the start of the line on one side, (1, 1) its end on
        // the other side, and so on.
        constexpr std::array<glm::vec2, VerticesPerLine> StripCorners = {
            glm::vec2(0.0f, -1.0f), glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 1.0f),
        };
    }

    DebugLineRenderer::DebugLineRenderer()
        : m_vertexBuffer(GLBuffer::CreateDynamic(InitialVertexCapacity * sizeof(StripVertex)))
    {
        m_vertexCapacity = InitialVertexCapacity;
        m_vertexArray.SetVertexBuffer(VertexBufferBinding, m_vertexBuffer, sizeof(StripVertex));
        m_vertexArray.SetFloatAttribute(DebugLineStartAttribute, VertexBufferBinding, 3, offsetof(StripVertex, lineStart));
        m_vertexArray.SetFloatAttribute(DebugLineEndAttribute, VertexBufferBinding, 3, offsetof(StripVertex, lineEnd));
        m_vertexArray.SetFloatAttribute(DebugLineColorAttribute, VertexBufferBinding, 3, offsetof(StripVertex, color));
        m_vertexArray.SetFloatAttribute(DebugLineCornerAttribute, VertexBufferBinding, 2, offsetof(StripVertex, corner));
    }

    void DebugLineRenderer::Draw(const DebugLines& lines, const View& view, const GLShaderProgram& program,
                                 glm::vec2 viewportSize, float lineWidth)
    {
        // The depth-tested strips first, then the ones on top, all in one buffer; each pass draws its own range.
        m_stripVertices.clear();
        AddStrips(lines.GetVertices(DebugLineDepth::Tested));
        const std::size_t testedVertexCount = m_stripVertices.size();
        AddStrips(lines.GetVertices(DebugLineDepth::OnTop));
        if (m_stripVertices.empty())
            return;

        if (m_stripVertices.size() > m_vertexCapacity)
            CreateBuffer(m_stripVertices.size() * 2);
        m_vertexBuffer.Update(std::as_bytes(std::span(m_stripVertices)));

        program.Use();
        program.SetUniform(ViewUniform, view.viewMatrix);
        program.SetUniform(ProjectionUniform, view.projectionMatrix);
        program.SetUniform(DebugLineViewportSizeUniform, viewportSize);
        program.SetUniform(DebugLineWidthUniform, lineWidth);
        m_vertexArray.Bind();

        // Seen from one side or the other, the corners of a strip can go clockwise or counter-clockwise: no culling.
        glDisable(GL_CULL_FACE);

        glEnable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(testedVertexCount));

        // Without the depth test nothing can hide these strips.
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLES, static_cast<GLint>(testedVertexCount),
                     static_cast<GLsizei>(m_stripVertices.size() - testedVertexCount));
        glEnable(GL_DEPTH_TEST);
    }

    void DebugLineRenderer::AddStrips(std::span<const DebugLineVertex> lineVertices)
    {
        for (std::size_t index = 0; index + 1 < lineVertices.size(); index += 2)
        {
            const DebugLineVertex& start = lineVertices[index];
            const DebugLineVertex& end = lineVertices[index + 1];
            for (const glm::vec2& corner : StripCorners)
                m_stripVertices.push_back(
                    {.lineStart = start.position, .lineEnd = end.position, .color = start.color, .corner = corner});
        }
    }

    void DebugLineRenderer::CreateBuffer(std::size_t vertexCapacity)
    {
        // The size of a buffer cannot change, so a bigger one replaces it; the vertex array must point to the new one.
        m_vertexBuffer = GLBuffer::CreateDynamic(vertexCapacity * sizeof(StripVertex));
        m_vertexArray.SetVertexBuffer(VertexBufferBinding, m_vertexBuffer, sizeof(StripVertex));
        m_vertexCapacity = vertexCapacity;
    }
}
