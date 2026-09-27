#include "Renderer/DebugLines.h"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

#include <cstddef>
#include <span>

namespace Abomination::Renderer
{
    TEST(DebugLines, LineAddsTwoVerticesWithItsColor)
    {
        DebugLines lines;

        lines.AddLine({0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 3.0f}, {1.0f, 0.0f, 0.0f});

        ASSERT_EQ(lines.GetVertices(DebugLineDepth::Tested).size(), 2u);
        EXPECT_EQ(lines.GetLineCount(), 1u);
        EXPECT_EQ(lines.GetVertices(DebugLineDepth::Tested)[1].position, glm::vec3(1.0f, 2.0f, 3.0f));
        EXPECT_EQ(lines.GetVertices(DebugLineDepth::Tested)[0].color, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    TEST(DebugLines, BoxIsTwelveEdgesBetweenItsCorners)
    {
        DebugLines lines;

        lines.AddBox({0.0f, 0.0f, 0.0f}, {1.0f, 2.0f, 3.0f}, {0.0f, 1.0f, 0.0f});

        ASSERT_EQ(lines.GetLineCount(), 12u);
        for (std::size_t index = 0; index < lines.GetVertices(DebugLineDepth::Tested).size(); index += 2)
        {
            const glm::vec3 from = lines.GetVertices(DebugLineDepth::Tested)[index].position;
            const glm::vec3 to = lines.GetVertices(DebugLineDepth::Tested)[index + 1].position;

            // Every edge runs along one axis: its ends differ in exactly one coordinate.
            const int differentCoordinates = (from.x != to.x) + (from.y != to.y) + (from.z != to.z);
            EXPECT_EQ(differentCoordinates, 1);

            // Both ends are corners: every coordinate is the minimum or the maximum of the box.
            for (const glm::vec3& corner : {from, to})
            {
                EXPECT_TRUE(corner.x == 0.0f || corner.x == 1.0f);
                EXPECT_TRUE(corner.y == 0.0f || corner.y == 2.0f);
                EXPECT_TRUE(corner.z == 0.0f || corner.z == 3.0f);
            }
        }
    }

    TEST(DebugLines, ClearRemovesAllLines)
    {
        DebugLines lines;
        lines.AddBox({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f});
        lines.AddLine({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DebugLineDepth::OnTop);

        lines.Clear();

        EXPECT_EQ(lines.GetLineCount(), 0u);
        EXPECT_TRUE(lines.GetVertices(DebugLineDepth::Tested).empty());
        EXPECT_TRUE(lines.GetVertices(DebugLineDepth::OnTop).empty());
    }

    TEST(DebugLines, LinesOnTopAreKeptApart)
    {
        DebugLines lines;

        lines.AddLine({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        lines.AddLine({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DebugLineDepth::OnTop);

        EXPECT_EQ(lines.GetVertices(DebugLineDepth::Tested).size(), 2u);
        EXPECT_EQ(lines.GetVertices(DebugLineDepth::OnTop).size(), 2u);
        EXPECT_EQ(lines.GetLineCount(), 2u);
    }

    TEST(DebugLines, ArrowIsLineAndFourHeadLinesFromTip)
    {
        DebugLines lines;
        const glm::vec3 tip{0.0f, 0.0f, 5.0f};

        lines.AddArrow({0.0f, 0.0f, 0.0f}, tip, {1.0f, 0.0f, 0.0f});

        const std::span<const DebugLineVertex> vertices = lines.GetVertices(DebugLineDepth::Tested);
        ASSERT_EQ(lines.GetLineCount(), 5u);
        for (std::size_t index = 2; index < vertices.size(); index += 2)
        {
            // Every head line starts at the tip and goes back towards the start (a smaller z), off the arrow's axis.
            EXPECT_EQ(vertices[index].position, tip);
            EXPECT_LT(vertices[index + 1].position.z, tip.z);
            EXPECT_GT(glm::length(glm::vec2(vertices[index + 1].position)), 0.0f);
        }
    }
}
