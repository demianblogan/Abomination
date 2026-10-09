#include "World/LightSources.h"

#include "Renderer/ColorSpace.h"
#include "World/MapCoordinates.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace Abomination::World
{
    namespace
    {
        using Renderer::FlameKind;

        // The fire offsets and the flames of the candles as MakeLightSources.py prints them ("FIRE Torch: ...",
        // "FLAME Candles: ..."); the flames of the torch and the brazier stand on its cloth head and on its coals, a little
        // below their fire. The light of a fire is warm: deep orange for burning wood and coals, more yellow for wax and the
        // oil of a lantern. Wood and coals flicker strongly, wicks hardly.
        constexpr std::array LightSourceTypes = {
            LightSourceType{.className = "torch",
                            .name = "Torch",
                            .modelPath = "Models/Props/Torch.glb",
                            .outModelPath = "Models/Props/TorchOut.glb",
                            .fireOffset = {0.0f, 0.303f, 0.128f},
                            .color = {255.0f, 140.0f, 60.0f},
                            .intensity = 8.0f,
                            .range = 8.0f,
                            .flickerStrength = 0.25f,
                            .flickerSpeed = 9.0f,
                            .flameKind = FlameKind::Wild,
                            .flameHeight = 0.28f,
                            .flameOffsets = {glm::vec3(0.0f, 0.27f, 0.115f)},
                            .flameCount = 1,
                            .sparksPerSecond = 3.0f,
                            .smokePerSecond = 2.0f},
            LightSourceType{.className = "brazier",
                            .name = "Brazier",
                            .modelPath = "Models/Props/Brazier.glb",
                            .outModelPath = "Models/Props/BrazierOut.glb",
                            .fireOffset = {-0.014f, 0.63f, 0.0f},
                            .color = {255.0f, 120.0f, 40.0f},
                            .intensity = 15.0f,
                            .range = 12.0f,
                            .flickerStrength = 0.2f,
                            .flickerSpeed = 7.0f,
                            .flameKind = FlameKind::Wild,
                            .flameHeight = 0.4f,
                            .flameOffsets = {glm::vec3(-0.014f, 0.58f, 0.0f), glm::vec3(0.12f, 0.57f, 0.06f),
                                             glm::vec3(-0.11f, 0.57f, -0.07f)},
                            .flameCount = 3,
                            .sparksPerSecond = 4.0f,
                            .smokePerSecond = 1.5f},
            LightSourceType{.className = "candles",
                            .name = "Candles",
                            .modelPath = "Models/Props/Candles.glb",
                            .outModelPath = "Models/Props/CandlesOut.glb",
                            .fireOffset = {0.01f, 0.122f, 0.0f},
                            .color = {255.0f, 170.0f, 90.0f},
                            .intensity = 3.0f,
                            .range = 5.0f,
                            .flickerStrength = 0.06f,
                            .flickerSpeed = 5.0f,
                            .flameKind = FlameKind::Calm,
                            .flameHeight = 0.06f,
                            .flameOffsets = {glm::vec3(0.0f, 0.128f, 0.0f), glm::vec3(0.07f, 0.048f, -0.03f),
                                             glm::vec3(-0.055f, -0.002f, -0.045f), glm::vec3(0.02f, 0.078f, 0.07f)},
                            .flameCount = 4},
            LightSourceType{.className = "lantern",
                            .name = "Lantern",
                            .modelPath = "Models/Props/Lantern.glb",
                            .outModelPath = "Models/Props/LanternOut.glb",
                            .fireOffset = {0.0f, -0.066f, 0.0f},
                            .color = {255.0f, 170.0f, 80.0f},
                            .intensity = 5.0f,
                            .range = 7.0f,
                            .flickerStrength = 0.08f,
                            .flickerSpeed = 5.0f},
        };

        // A phase (radians) that every place gives differently but always the same: two torches never flicker together,
        // and a torch flickers the same way every time the level is loaded. The fractional part of a large multiple of a
        // sine is the usual cheap way to turn a number into a random-looking one.
        float CalculatePhase(const glm::vec3& position, float salt)
        {
            const float value = std::sin(glm::dot(position, glm::vec3(12.9898f, 78.233f, 37.719f)) + salt) * 43758.547f;
            return (value - std::floor(value)) * glm::two_pi<float>();
        }
    }

    std::span<const LightSourceType> GetLightSourceTypes()
    {
        return LightSourceTypes;
    }

    std::optional<MapLightSource> ReadMapLightSource(const MapEntity& entity)
    {
        const std::string* className = FindProperty(entity, "classname");
        if (className == nullptr)
            return std::nullopt;

        const auto type = std::ranges::find(LightSourceTypes, *className, &LightSourceType::className);
        if (type == LightSourceTypes.end())
            return std::nullopt;

        // Spawnflags are bits; the editor writes them as one number.
        const auto flags = static_cast<int>(ReadNumberProperty(entity, "spawnflags", 0.0));
        const bool isOut = (flags & LightSourceOutFlag) != 0;

        MapLightSource source{
            .name = std::string(type->name) + (isOut ? " (out)" : ""),
            .modelPath = std::string(isOut ? type->outModelPath : type->modelPath),
            .transform = ReadMapModelTransform(entity),
        };

        if (isOut)
            return source;

        // Offsets in the model are turned with it and moved to where it stands.
        const auto place = [&source](const glm::vec3& offset)
        {
            return source.transform.position + source.transform.rotation * offset;
        };

        // The light starts as the type gives it; the entity may change it like any light.
        MapLight light;
        light.light.color = Renderer::ConvertSRGBToLinear(type->color / 255.0f);
        light.light.intensity = type->intensity;
        light.light.range = type->range;
        ReadLightProperties(entity, light.light);
        light.transform.position = place(type->fireOffset);
        source.light = light;

        source.flicker = Renderer::Flicker{
            .baseIntensity = light.light.intensity,
            .basePosition = light.transform.position,
            .strength = type->flickerStrength,
            .speed = type->flickerSpeed,
            .phase = CalculatePhase(light.transform.position, 0.0f),
        };

        for (int index = 0; index < type->flameCount; ++index)
        {
            const glm::vec3 position = place(type->flameOffsets[index]);
            source.flames.push_back({
                .position = position,
                .flame = Renderer::Flame{
                    .kind = type->flameKind,
                    .height = type->flameHeight,
                    .phase = CalculatePhase(position, 1.0f),
                    .sparksPerSecond = type->sparksPerSecond,
                    .smokePerSecond = type->smokePerSecond,
                },
            });
        }

        return source;
    }
}
