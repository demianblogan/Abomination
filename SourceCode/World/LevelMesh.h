#pragma once

#include "Renderer/Assets/MeshData.h"
#include "World/MapData.h"

#include <glm/vec2.hpp>

#include <functional>
#include <string>
#include <vector>

namespace Abomination::World
{
    // Numbers about the geometry of a level, for the debug overlay.
    struct LevelMeshStatistics
    {
        int brushCount = 0;
        int faceCount = 0;
        int triangleCount = 0;
    };

    // The faces of the level that have one texture, ready to become one mesh. One draw call draws with one texture, so the
    // level is split into one part per texture.
    struct LevelMeshPart
    {
        // As the map writes it: "Episode1/Wall_MossyBrick".
        std::string textureName;
        Renderer::MeshData data;
    };

    // The geometry of all brushes of an entity (the world), split by texture, and the numbers about it.
    struct LevelMesh
    {
        // In the order the textures first appear in the map, so the same map always gives the same parts.
        std::vector<LevelMeshPart> parts;
        LevelMeshStatistics statistics;
    };

    // Gives the size in texels of the texture with this name (as the map writes it: "Episode1/Wall_MossyBrick").
    // Texture coordinates depend on it, and the texture files are known only to the caller (the texture store).
    using TextureSizeLookup = std::function<glm::ivec2(const std::string& textureName)>;

    // Builds the geometry of all brushes of an entity in the game's meters and axes: every face is built from its
    // planes (BuildBrushPolygons) and cut into triangles. Every vertex gets the normal of its face and its texture
    // coordinates (CalculateTextureCoordinates, with the texture size from getTextureSize); every face goes to the part of
    // its texture.
    [[nodiscard]] LevelMesh BuildLevelMesh(const MapEntity& entity, const TextureSizeLookup& getTextureSize);
}
