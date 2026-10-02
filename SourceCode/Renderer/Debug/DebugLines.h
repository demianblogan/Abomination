#pragma once

#include <glm/vec3.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace Abomination::Renderer
{
    // One end of a debug line.
    struct DebugLineVertex
    {
        glm::vec3 position{0.0f};
        glm::vec3 color{1.0f};
    };

    // Whether a debug line can be hidden by the scene.
    enum class DebugLineDepth
    {
        // Hidden behind walls like everything else, so it is clear where in space the line is.
        HiddenBehindWalls,

        // Always visible, drawn over everything: for markers that must never be lost (the world axes).
        AlwaysVisible,
    };

    // Lines drawn over the scene for debugging: collision boxes, traces, normals, axes. Any code adds lines during the
    // frame; the renderer draws them all at once (DebugLineRenderer) and clears the list for the next frame.
    // Plain memory, no OpenGL: it can be filled (and tested) anywhere.
    class DebugLines
    {
    public:
        void AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color,
                     DebugLineDepth depth = DebugLineDepth::HiddenBehindWalls);

        // The 12 edges of an axis-aligned box.
        void AddBox(const glm::vec3& minimum, const glm::vec3& maximum, const glm::vec3& color,
                    DebugLineDepth depth = DebugLineDepth::HiddenBehindWalls);

        // A line with an arrow head at its end: four short lines from the tip back, like the edges of a pyramid.
        void AddArrow(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color,
                      DebugLineDepth depth = DebugLineDepth::HiddenBehindWalls);

        void Clear() noexcept;

        // The lines of one depth mode, two vertices per line: the ends of line i are vertices 2i and 2i + 1.
        [[nodiscard]] std::span<const DebugLineVertex> GetVertices(DebugLineDepth depth) const noexcept;

        // All lines of both depth modes.
        [[nodiscard]] std::size_t GetLineCount() const noexcept;

    private:
        std::vector<DebugLineVertex> m_hiddenBehindWallsVertices;
        std::vector<DebugLineVertex> m_alwaysVisibleVertices;
    };
}
