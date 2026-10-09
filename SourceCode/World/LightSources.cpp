#include "World/LightSources.h"

#include "Renderer/ColorSpace.h"
#include "World/MapCoordinates.h"

#include <algorithm>
#include <array>

namespace Abomination::World
{
    namespace
    {
        // The fire offsets as MakeLightSources.py prints them ("FIRE Torch: ..."). The light of a fire is warm: deep
        // orange for burning wood and coals, more yellow for wax and the oil of a lantern.
        constexpr std::array LightSourceTypes = {
            LightSourceType{.className = "torch",
                            .name = "Torch",
                            .modelPath = "Models/Props/Torch.glb",
                            .outModelPath = "Models/Props/TorchOut.glb",
                            .fireOffset = {0.0f, 0.303f, 0.128f},
                            .color = {255.0f, 140.0f, 60.0f},
                            .intensity = 8.0f,
                            .range = 8.0f},
            LightSourceType{.className = "brazier",
                            .name = "Brazier",
                            .modelPath = "Models/Props/Brazier.glb",
                            .outModelPath = "Models/Props/BrazierOut.glb",
                            .fireOffset = {-0.014f, 0.63f, 0.0f},
                            .color = {255.0f, 120.0f, 40.0f},
                            .intensity = 15.0f,
                            .range = 12.0f},
            LightSourceType{.className = "candles",
                            .name = "Candles",
                            .modelPath = "Models/Props/Candles.glb",
                            .outModelPath = "Models/Props/CandlesOut.glb",
                            .fireOffset = {0.01f, 0.122f, 0.0f},
                            .color = {255.0f, 170.0f, 90.0f},
                            .intensity = 3.0f,
                            .range = 5.0f},
            LightSourceType{.className = "lantern",
                            .name = "Lantern",
                            .modelPath = "Models/Props/Lantern.glb",
                            .outModelPath = "Models/Props/LanternOut.glb",
                            .fireOffset = {0.0f, -0.066f, 0.0f},
                            .color = {255.0f, 170.0f, 80.0f},
                            .intensity = 5.0f,
                            .range = 7.0f},
        };
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

        // The light starts as the type gives it; the entity may change it like any light. The fire turns with the model.
        MapLight light;
        light.light.color = Renderer::ConvertSRGBToLinear(type->color / 255.0f);
        light.light.intensity = type->intensity;
        light.light.range = type->range;
        ReadLightProperties(entity, light.light);
        light.transform.position = source.transform.position + source.transform.rotation * type->fireOffset;
        source.light = light;

        return source;
    }
}
