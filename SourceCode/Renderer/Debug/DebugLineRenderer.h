#pragma once

#include "Renderer/Camera/View.h"
#include "Renderer/Debug/DebugLines.h"
#include "Renderer/OpenGL/GLBuffer.h"
#include "Renderer/OpenGL/GLShaderProgram.h"
#include "Renderer/OpenGL/GLVertexArray.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace Abomination::Renderer
{
    // Draws DebugLines as lines several pixels wide.
    //
    // OpenGL can draw lines (GL_LINES), but in the Core profile drivers only have to support a width of 1 pixel, which is
    // hard to see. So every line becomes a thin strip of two triangles instead. The strip is widened in the vertex
    // shader, on the screen: it stays the same number of pixels wide however far away the line is.
    //
    // Lines are drawn in two passes: first the depth-tested ones, then the ones on top of everything (see
    // DebugLineDepth). Everything goes into one dynamic vertex buffer that is reused every frame; when a frame has more
    // lines than fit, the buffer is replaced by one twice as big. Requires a current OpenGL context. Move-only.
    class DebugLineRenderer
    {
    public:
        DebugLineRenderer();

        // Draws the lines with the program (Shaders/DebugLines), as seen from the view. viewportSize is the size of the
        // frame in pixels; lineWidth is the width of the lines in pixels.
        void Draw(const DebugLines& lines, const View& view, const GLShaderProgram& program, glm::vec2 viewportSize,
                  float lineWidth);

    private:
        // A corner of the strip of a line. Every one knows both ends of its line, so the vertex shader can find the
        // direction of the line on the screen and move the corner across it.
        struct StripVertex
        {
            glm::vec3 lineStart{0.0f};
            glm::vec3 lineEnd{0.0f};
            glm::vec3 color{1.0f};

            // x: 0 = the corner is at the start of the line, 1 = at its end; y: -1 or 1 = the side of the line.
            glm::vec2 corner{0.0f};
        };

        // Adds the 6 corners (2 triangles) of every line to m_stripVertices.
        void AddStrips(std::span<const DebugLineVertex> lineVertices);

        // Creates the buffer for vertexCapacity strip vertices and connects it to the vertex array.
        void CreateBuffer(std::size_t vertexCapacity);

        // Kept between frames so its memory is reused.
        std::vector<StripVertex> m_stripVertices;
        std::size_t m_vertexCapacity = 0;

        // The vertex array only refers to the buffer, so it is declared after it and destroyed before it.
        GLBuffer m_vertexBuffer;
        GLVertexArray m_vertexArray;
    };
}
