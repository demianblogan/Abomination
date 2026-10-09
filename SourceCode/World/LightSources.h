#pragma once

#include "Core/Scene/Transform.h"
#include "Renderer/Fire.h"
#include "World/MapData.h"
#include "World/MapLights.h"

#include <glm/vec3.hpp>

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

        // How its light wavers (see Renderer::Flicker).
        float flickerStrength = 0.2f;
        float flickerSpeed = 8.0f;

        // Its flames: where each stands (the bottom of the flame, in the coordinates of the model like fireOffset), how
        // tall they are, and the sparks and smoke of each. A lantern has none: its flame is behind its panes.
        Renderer::FlameKind flameKind = Renderer::FlameKind::Wild;
        float flameHeight = 0.25f;
        std::array<glm::vec3, 4> flameOffsets{};
        int flameCount = 0;
        float sparksPerSecond = 0.0f;
        float smokePerSecond = 0.0f;
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

        // Where its fire is and what it gives, and how it wavers; nothing when the fire is out.
        std::optional<MapLight> light;
        Renderer::Flicker flicker;

        // Its flames, each placed at the bottom of the flame (none when the fire is out).
        struct PlacedFlame
        {
            glm::vec3 position{0.0f};
            Renderer::Flame flame;
        };
        std::vector<PlacedFlame> flames;
    };

    // The light source of a torch, brazier, candles or lantern entity, or nothing for any other entity.
    [[nodiscard]] std::optional<MapLightSource> ReadMapLightSource(const MapEntity& entity);
}
