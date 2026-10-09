#include "World/LightSources.h"

#include <gtest/gtest.h>

namespace Abomination::World
{
    namespace
    {
        MapEntity MakeEntity(std::unordered_map<std::string, std::string> properties)
        {
            return MapEntity{.properties = std::move(properties)};
        }

        constexpr float Tolerance = 1e-4f;
    }

    TEST(LightSources, OtherEntitiesAreNoLightSources)
    {
        EXPECT_FALSE(ReadMapLightSource(MakeEntity({{"classname", "point_light"}})).has_value());
        EXPECT_FALSE(ReadMapLightSource(MakeEntity({})).has_value());
    }

    TEST(LightSources, TorchBringsItsModelAndItsLightAtTheFire)
    {
        const std::optional<MapLightSource> source =
            ReadMapLightSource(MakeEntity({{"classname", "torch"}, {"origin", "64 0 32"}, {"angle", "0"}}));
        ASSERT_TRUE(source.has_value());

        EXPECT_EQ(source->name, "Torch");
        EXPECT_EQ(source->modelPath, "Models/Props/Torch.glb");
        ASSERT_TRUE(source->light.has_value());

        // Facing map +X (game +X), the fire is in front of the middle of the model (+Z of the model) and above it.
        const glm::vec3 fire = source->light->transform.position - source->transform.position;
        EXPECT_NEAR(fire.x, 0.128f, Tolerance);
        EXPECT_NEAR(fire.y, 0.303f, Tolerance);
        EXPECT_NEAR(fire.z, 0.0f, Tolerance);
        EXPECT_FLOAT_EQ(source->light->light.intensity, 8.0f);
    }

    TEST(LightSources, OutHasNoLight)
    {
        const std::optional<MapLightSource> source =
            ReadMapLightSource(MakeEntity({{"classname", "brazier"}, {"spawnflags", "1"}}));
        ASSERT_TRUE(source.has_value());

        EXPECT_EQ(source->name, "Brazier (out)");
        EXPECT_EQ(source->modelPath, "Models/Props/BrazierOut.glb");
        EXPECT_FALSE(source->light.has_value());
    }

    TEST(LightSources, EntityMayChangeTheLight)
    {
        const std::optional<MapLightSource> source = ReadMapLightSource(
            MakeEntity({{"classname", "candles"}, {"intensity", "1.5"}, {"range", "64"}}));
        ASSERT_TRUE(source.has_value());
        ASSERT_TRUE(source->light.has_value());

        EXPECT_FLOAT_EQ(source->light->light.intensity, 1.5f);
        EXPECT_FLOAT_EQ(source->light->light.range, 2.0f);
    }

    TEST(LightSources, FlamesBurnOnlyWhereTheyAreSeen)
    {
        // A flame for every wick of the candles, none for the lantern (behind its panes), none when the fire is out.
        const auto countFlames = [](std::unordered_map<std::string, std::string> properties)
        {
            const std::optional<MapLightSource> source = ReadMapLightSource(MakeEntity(std::move(properties)));
            return source.has_value() ? source->flames.size() : 99u;
        };

        EXPECT_EQ(countFlames({{"classname", "candles"}}), 4u);
        EXPECT_EQ(countFlames({{"classname", "lantern"}}), 0u);
        EXPECT_EQ(countFlames({{"classname", "torch"}, {"spawnflags", "1"}}), 0u);
    }

    TEST(LightSources, FlickerStartsFromTheLightOfTheEntity)
    {
        const std::optional<MapLightSource> source =
            ReadMapLightSource(MakeEntity({{"classname", "torch"}, {"intensity", "12"}}));
        ASSERT_TRUE(source.has_value());
        ASSERT_TRUE(source->light.has_value());

        EXPECT_FLOAT_EQ(source->flicker.baseIntensity, 12.0f);
        EXPECT_EQ(source->flicker.basePosition, source->light->transform.position);
    }

    TEST(LightSources, TorchesAndBraziersAreInTheWayCandlesAreNot)
    {
        const std::optional<MapLightSource> torch =
            ReadMapLightSource(MakeEntity({{"classname", "torch"}, {"spawnflags", "1"}}));
        ASSERT_TRUE(torch.has_value());
        ASSERT_TRUE(torch->collisionBrush.has_value());

        // A box of clip: characters bump into it, shots pass through. Even burnt out, the torch is still there.
        EXPECT_EQ(torch->collisionBrush->faces.size(), 6u);
        EXPECT_TRUE(IsClipBrush(*torch->collisionBrush));

        const std::optional<MapLightSource> candles = ReadMapLightSource(MakeEntity({{"classname", "candles"}}));
        ASSERT_TRUE(candles.has_value());
        EXPECT_FALSE(candles->collisionBrush.has_value());
    }

    TEST(LightSources, ClassNamesAreUnique)
    {
        const std::span<const LightSourceType> types = GetLightSourceTypes();
        for (std::size_t first = 0; first < types.size(); ++first)
            for (std::size_t second = first + 1; second < types.size(); ++second)
                EXPECT_NE(types[first].className, types[second].className);
    }
}
