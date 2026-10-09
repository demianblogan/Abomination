#include "Core/Scene/Transform.h"
#include "Renderer/Light.h"
#include "Renderer/LightBuffer.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <gtest/gtest.h>

#include <cmath>

namespace Abomination::Renderer
{
    namespace
    {
        constexpr float Tolerance = 1e-5f;
    }

    TEST(LightBuffer, LightsAreSeenFromTheCamera)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();
        registry.emplace<Core::Transform>(entity, Core::Transform{.position = {1.0f, 2.0f, 3.0f}});
        registry.emplace<Light>(entity, Light{.color = {1.0f, 0.5f, 0.0f}, .intensity = 4.0f, .range = 6.0f});

        // A camera 10 m along +Z of the world: the light is 7 m in front of it (-Z), 1 m right, 2 m up.
        const glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
        std::vector<ShaderLight> lights;
        GatherShaderLights(registry, viewMatrix, 1.0f, lights);

        ASSERT_EQ(lights.size(), 1u);
        EXPECT_EQ(lights[0].positionAndRange, glm::vec4(1.0f, 2.0f, -7.0f, 6.0f));

        // The color is multiplied by the intensity; a point light is type 0.
        EXPECT_EQ(lights[0].colorAndType, glm::vec4(4.0f, 2.0f, 0.0f, 0.0f));
    }

    TEST(LightBuffer, SpotCarriesItsDirectionAndTheCosinesOfItsCones)
    {
        entt::registry registry;
        const entt::entity entity = registry.create();

        // Turned 90 degrees around the vertical: its forward (-Z) becomes -X.
        registry.emplace<Core::Transform>(
            entity, Core::Transform{.rotation = glm::angleAxis(glm::radians(90.0f), Core::WorldUp)});
        registry.emplace<Light>(entity, Light{.type = LightType::Spot,
                                              .innerConeAngle = glm::radians(20.0f),
                                              .outerConeAngle = glm::radians(30.0f)});

        std::vector<ShaderLight> lights;
        GatherShaderLights(registry, glm::mat4(1.0f), 1.0f, lights);

        ASSERT_EQ(lights.size(), 1u);
        EXPECT_FLOAT_EQ(lights[0].colorAndType.w, 1.0f);
        EXPECT_NEAR(lights[0].directionAndOuterCosine.x, -1.0f, Tolerance);
        EXPECT_NEAR(lights[0].directionAndOuterCosine.w, std::cos(glm::radians(30.0f)), Tolerance);
        EXPECT_NEAR(lights[0].innerCosine.x, std::cos(glm::radians(20.0f)), Tolerance);
    }
}
