#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// The contents of a .map file as it is written, before any geometry is calculated. TrenchBroom writes these files;
// World::ParseMap reads them. Everything is in the coordinates and units of the map (Quake units, Z up); converting to
// the game's meters and axes happens later, when the level geometry is built.
//
// Doubles are used because TrenchBroom writes points with 15-17 significant digits (a rotated brush has corners like
// -301.25483399593907), and the planes built from them must stay exact: floats keep only about 7 digits.
namespace Abomination::World
{
    // One face of a brush, one line of the file in the Valve 220 format:
    //   ( x y z ) ( x y z ) ( x y z ) texture [ ux uy uz offsetU ] [ vx vy vz offsetV ] rotation scaleU scaleV
    struct MapFace
    {
        // Three points on the plane of the face. Their order tells which side of the plane is the outside of the brush.
        std::array<glm::dvec3, 3> points{};

        // The texture as a path in Assets/Textures without the extension: "Episode1/Wall_MossyBrick" for
        // Assets/Textures/Episode1/Wall_MossyBrick.png.
        std::string textureName;

        // How the texture lies on the face (used when textures are drawn): the directions of the texture's U (across)
        // and V (down) axes in the world, the offsets along them in texels, the rotation in degrees (already included in
        // the axes, kept for editors) and the scale (world units per texel).
        glm::dvec3 textureUAxis{0.0};
        glm::dvec3 textureVAxis{0.0};
        double textureOffsetU = 0.0;
        double textureOffsetV = 0.0;
        double textureRotation = 0.0;
        double textureScaleU = 1.0;
        double textureScaleV = 1.0;
    };

    // A convex solid given by the planes of its faces: its inside is behind all of them.
    struct MapBrush
    {
        std::vector<MapFace> faces;
    };

    // An entity of the map: key-value properties ("classname" says what it is) and, for solid entities (the world,
    // later doors and triggers), its brushes. Point entities (the player start) have no brushes.
    struct MapEntity
    {
        std::unordered_map<std::string, std::string> properties;
        std::vector<MapBrush> brushes;
    };

    struct MapData
    {
        std::vector<MapEntity> entities;
    };

    // The value of a property, or nullptr if the entity does not have it.
    [[nodiscard]] const std::string* FindProperty(const MapEntity& entity, const std::string& key);

    // The number of a property, or fallback if the entity does not have it or it is not a number.
    [[nodiscard]] double ReadNumberProperty(const MapEntity& entity, const std::string& key, double fallback);

    // The texture of clip brushes: invisible walls and ramps that only characters collide with, as in Quake. A ramp of
    // clip laid over a stair lets characters walk up and down it smoothly, while the steps are still seen. Shots,
    // shells and sight go through clip (see Level::GetShotBrushes).
    inline constexpr std::string_view ClipTextureName = "Common/Clip";

    // Whether a brush is clip: any of its faces has the clip texture.
    [[nodiscard]] bool IsClipBrush(const MapBrush& brush);
}
