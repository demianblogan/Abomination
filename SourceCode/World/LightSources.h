#pragma once

#include "Core/Scene/Transform.h"
#include "World/MapData.h"
#include "World/MapLights.h"

#include <glm/vec3.hpp>

#include <optional>
#include <span>
#include <string>
#include <string_view>

// The light sources of a map placed as one entity each (Tools/TrenchBroom/Abomination/Abomination.fgd): a torch, a
// brazier, candles, a lantern. The entity brings its model and its light, placed where the fire of the model burns, so a
// level designer places one thing instead of a model and a light that must be kept together. The spawnflag Out puts the
// fire out: the model with ash and dark panes, and no light.
//
// The models are made by Tools/Blender/MakeLightSources.py, which prints where their fire is and how large their box is;
// the numbers below and in the entity definitions come from it. A model has the middle of its box at its origin, and the
// entity box of the editor is that box, so a torch placed with its box against a wall touches the wall.
namespace Abomination::World
{
    // What a kind of light source is.
    struct LightSourceType
    {
        // The class name of the entity ("torch") and the name of its entities in the game ("Torch").
        std::string_view className;
        std::string_view name;

        // The model burning and the model with its fire out (paths in Assets).
        std::string_view modelPath;
        std::string_view outModelPath;

        // Where the fire burns, in meters from the middle of the model, in its own coordinates (+Y up, +Z its front).
        glm::vec3 fireOffset{0.0f};

        // The light of its fire unless the entity says otherwise (see ReadLightProperties): an sRGB color as the editor
        // picks it (0..255), the light at 1 m, and the range in meters.
        glm::vec3 color{255.0f};
        float intensity = 5.0f;
        float range = 10.0f;
    };

    // Every kind, in the order of the entity definitions.
    [[nodiscard]] std::span<const LightSourceType> GetLightSourceTypes();

    // The spawnflag of a light source whose fire is out.
    inline constexpr int LightSourceOutFlag = 1;

    // A light source as the level places it.
    struct MapLightSource
    {
        // "Torch", or "Torch (out)".
        std::string name;

        std::string modelPath;
        Core::Transform transform;

        // Where its fire is and what it gives; nothing when the fire is out.
        std::optional<MapLight> light;
    };

    // The light source of a torch, brazier, candles or lantern entity, or nothing for any other entity.
    [[nodiscard]] std::optional<MapLightSource> ReadMapLightSource(const MapEntity& entity);
}
