#include "Renderer/Debug/DebugLines.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <array>

namespace Abomination::Renderer
{
    namespace
    {
        // The arrow head: its length as a part of the arrow, and its width as a part of the head length.
        constexpr float ArrowHeadLengthRatio = 0.2f;
        constexpr float ArrowHeadWidthRatio = 0.35f;
    }

    void DebugLines::AddLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color, DebugLineDepth depth)
    {
        std::vector<DebugLineVertex>& vertices =
            depth == DebugLineDepth::AlwaysVisible ? m_alwaysVisibleVertices : m_hiddenBehindWallsVertices;
        vertices.push_back({.position = from, .color = color});
        vertices.push_back({.position = to, .color = color});
    }

    void DebugLines::AddBox(const glm::vec3& minimum, const glm::vec3& maximum, const glm::vec3& color,
                            DebugLineDepth depth)
    {
        // The 8 corners: bit 0 of the index picks x (minimum or maximum), bit 1 picks y, bit 2 picks z.
        std::array<glm::vec3, 8> corners;
        for (std::size_t index = 0; index < corners.size(); ++index)
            corners[index] = glm::vec3((index & 1) != 0 ? maximum.x : minimum.x, (index & 2) != 0 ? maximum.y : minimum.y,
                                       (index & 4) != 0 ? maximum.z : minimum.z);

        // Two corners share an edge when their indices differ in exactly one bit: 4 edges along each of the 3 axes.
        constexpr std::array<std::array<std::size_t, 2>, 12> Edges = {{
            {0, 1}, {2, 3}, {4, 5}, {6, 7}, // along x
            {0, 2}, {1, 3}, {4, 6}, {5, 7}, // along y
            {0, 4}, {1, 5}, {2, 6}, {3, 7}, // along z
        }};
        for (const std::array<std::size_t, 2>& edge : Edges)
            AddLine(corners[edge[0]], corners[edge[1]], color, depth);
    }

    void DebugLines::AddArrow(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color, DebugLineDepth depth)
    {
        AddLine(from, to, color, depth);

        const float length = glm::length(to - from);
        if (length <= 0.0f)
            return;

        // Two directions across the arrow, perpendicular to it and to each other. The cross product of the arrow with any
        // direction not parallel to it is across it; world up works unless the arrow is (nearly) vertical, then X does.
        const glm::vec3 direction = (to - from) / length;
        const glm::vec3 helper = glm::abs(direction.y) < 0.9f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 across = glm::normalize(glm::cross(direction, helper));
        const glm::vec3 acrossToo = glm::cross(direction, across);

        // The base of the head lies a bit back from the tip; four lines lead from the tip to the corners of the base.
        const float headLength = length * ArrowHeadLengthRatio;
        const float headWidth = headLength * ArrowHeadWidthRatio;
        const glm::vec3 baseCenter = to - direction * headLength;
        for (const glm::vec3& side : {across, -across, acrossToo, -acrossToo})
            AddLine(to, baseCenter + side * headWidth, color, depth);
    }

    void DebugLines::Clear() noexcept
    {
        // clear() keeps the allocated memory, so the next frame adds its lines without allocating again.
        m_hiddenBehindWallsVertices.clear();
        m_alwaysVisibleVertices.clear();
    }

    std::span<const DebugLineVertex> DebugLines::GetVertices(DebugLineDepth depth) const noexcept
    {
        return depth == DebugLineDepth::AlwaysVisible ? m_alwaysVisibleVertices : m_hiddenBehindWallsVertices;
    }

    std::size_t DebugLines::GetLineCount() const noexcept
    {
        return (m_hiddenBehindWallsVertices.size() + m_alwaysVisibleVertices.size()) / 2;
    }
}
