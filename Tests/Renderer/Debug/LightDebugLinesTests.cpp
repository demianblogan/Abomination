#include "Core/Scene/Transform.h"
#include "Renderer/Debug/LightDebugLines.h"
#include "Renderer/Light.h"

#include <gtest/gtest.h>

namespace Abomination::Renderer
{
    TEST(LightDebugLines, PointLightIsAStarWithThreeCircles)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Core::Transform>(entity);
        registry.emplace<Light>(entity, Light{.color = {0.5f, 0.0f, 0.0f}});

        DebugLines lines;
        AddLightDebugLines(registry, lines);

        // The star over everything (3 lines), the circles behind walls (32 pieces each).
        const std::span<const DebugLineVertex> star = lines.GetVertices(DebugLineDepth::AlwaysVisible);
        EXPECT_EQ(star.size(), 3u * 2u);
        EXPECT_EQ(lines.GetVertices(DebugLineDepth::HiddenBehindWalls).size(), 3u * 32u * 2u);

        // A dim light is still drawn as bright as a line can be.
        EXPECT_FLOAT_EQ(star[0].color.r, 1.0f);
        EXPECT_FLOAT_EQ(star[0].color.g, 0.0f);
    }

    TEST(LightDebugLines, SpotLightShowsItsCone)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Core::Transform>(entity);
        registry.emplace<Light>(entity, Light{.type = LightType::Spot});

        DebugLines lines;
        AddLightDebugLines(registry, lines);

        // The circle at the end of the cone and four lines to it.
        EXPECT_EQ(lines.GetVertices(DebugLineDepth::HiddenBehindWalls).size(), (32u + 4u) * 2u);
    }
}
