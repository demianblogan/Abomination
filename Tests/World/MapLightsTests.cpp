#include "Core/Scene/Transform.h"
#include "World/MapLights.h"

#include <glm/trigonometric.hpp>
#include <gtest/gtest.h>

namespace Abomination::World
{
    namespace
    {
        MapEntity MakeEntity(std::unordered_map<std::string, std::string> properties)
        {
            return MapEntity{.properties = std::move(properties)};
        }

        constexpr float Tolerance = 1e-5f;
    }

    TEST(MapLights, OtherEntitiesAreNoLights)
    {
        EXPECT_FALSE(ReadMapLight(MakeEntity({{"classname", "monster_dog"}})).has_value());
        EXPECT_FALSE(ReadMapLight(MakeEntity({})).has_value());
    }

    TEST(MapLights, PointLightTakesItsPropertiesInMeters)
    {
        const std::optional<MapLight> mapLight = ReadMapLight(MakeEntity({{"classname", "point_light"},
                                                                          {"origin", "64 0 32"},
                                                                          {"intensity", "8"},
                                                                          {"range", "160"},
                                                                          {"_color", "255 255 255"}}));
        ASSERT_TRUE(mapLight.has_value());

        // (x, y, z) of the map is (x, z, -y) / 32 in the game.
        EXPECT_NEAR(mapLight->transform.position.x, 2.0f, Tolerance);
        EXPECT_NEAR(mapLight->transform.position.y, 1.0f, Tolerance);
        EXPECT_NEAR(mapLight->transform.position.z, 0.0f, Tolerance);

        const Renderer::Light& light = mapLight->light;
        EXPECT_EQ(light.type, Renderer::LightType::Point);
        EXPECT_FLOAT_EQ(light.intensity, 8.0f);
        EXPECT_FLOAT_EQ(light.range, 5.0f);
        EXPECT_NEAR(light.color.g, 1.0f, Tolerance);
    }

    TEST(MapLights, MissingPropertiesKeepTheDefaultsOfTheEntityDefinition)
    {
        // Abomination.fgd: intensity 5, range 320 units, white.
        const std::optional<MapLight> mapLight =
            ReadMapLight(MakeEntity({{"classname", "point_light"}, {"range", "far"}}));
        ASSERT_TRUE(mapLight.has_value());

        EXPECT_FLOAT_EQ(mapLight->light.intensity, 5.0f);
        EXPECT_FLOAT_EQ(mapLight->light.range, 10.0f);
        EXPECT_EQ(mapLight->light.color, glm::vec3(1.0f));
    }

    TEST(MapLights, ColorPickedInTheEditorBecomesLinear)
    {
        const std::optional<MapLight> mapLight =
            ReadMapLight(MakeEntity({{"classname", "point_light"}, {"_color", "255 128 0"}}));
        ASSERT_TRUE(mapLight.has_value());

        // 128 of 255 in sRGB is about 22% of the light.
        EXPECT_NEAR(mapLight->light.color.r, 1.0f, Tolerance);
        EXPECT_NEAR(mapLight->light.color.g, 0.2159f, 1e-3f);
        EXPECT_NEAR(mapLight->light.color.b, 0.0f, Tolerance);
    }

    TEST(MapLights, SpotPointsAlongItsAngles)
    {
        const auto readDirection = [](const std::string& angles)
        {
            const std::optional<MapLight> mapLight =
                ReadMapLight(MakeEntity({{"classname", "spot_light"}, {"angles", angles}}));
            EXPECT_TRUE(mapLight.has_value());
            EXPECT_EQ(mapLight->light.type, Renderer::LightType::Spot);

            return mapLight->transform.rotation * Core::LocalForward;
        };

        // "pitch yaw roll" as TrenchBroom writes them: pitch 90 is straight down; yaw 0 looks along +X of the map, which
        // is +X of the game; yaw 90 along +Y of the map, which is -Z of the game.
        const glm::vec3 down = readDirection("90 0 0");
        EXPECT_NEAR(down.y, -1.0f, Tolerance);

        const glm::vec3 alongX = readDirection("0 0 0");
        EXPECT_NEAR(alongX.x, 1.0f, Tolerance);

        const glm::vec3 alongMapY = readDirection("0 90 0");
        EXPECT_NEAR(alongMapY.z, -1.0f, Tolerance);

        // Along +X, 45 degrees above the horizon.
        const glm::vec3 raised = readDirection("-45 0 0");
        EXPECT_NEAR(raised.x, raised.y, Tolerance);
        EXPECT_GT(raised.y, 0.0f);
    }

    TEST(MapLights, SpotWithoutAnglesShinesAlongX)
    {
        // As TrenchBroom shows it: an entity without angles is not turned.
        const std::optional<MapLight> mapLight = ReadMapLight(MakeEntity({{"classname", "spot_light"}}));
        ASSERT_TRUE(mapLight.has_value());

        EXPECT_NEAR((mapLight->transform.rotation * Core::LocalForward).x, 1.0f, Tolerance);
    }

    TEST(MapLights, InnerConeStaysInsideTheCone)
    {
        const std::optional<MapLight> mapLight = ReadMapLight(
            MakeEntity({{"classname", "spot_light"}, {"cone", "40"}, {"inner_cone", "50"}}));
        ASSERT_TRUE(mapLight.has_value());

        EXPECT_NEAR(mapLight->light.outerConeAngle, glm::radians(40.0f), Tolerance);
        EXPECT_NEAR(mapLight->light.innerConeAngle, glm::radians(40.0f), Tolerance);
    }
}
