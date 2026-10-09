#pragma once

#include "Core/Assets/AssetLifetime.h"
#include "Renderer/Assets/TextureStore.h"
#include "Renderer/Material.h"

#include <string>

namespace Abomination::Renderer
{
    // The maps of a material found by the names of their files, next to its color texture. The textures of the level have
    // no file describing them (that comes with the JSON configuration of 0.6), so the name is the description:
    //   Textures/Episode1/Wall_MossyBrick.png             the color
    //   Textures/Episode1/Wall_MossyBrick_Normal.png      the normal map
    //   Textures/Episode1/Wall_MossyBrick_MetalRough.png  roughness (green) and metalness (blue)
    //   Textures/Episode1/Wall_MossyBrick_Emissive.png    the light the surface gives off
    //   Textures/Episode1/Wall_MossyBrick_Height.png      the height, for parallax (a surface with it has parallax)
    struct MaterialFilePaths
    {
        std::string baseColor;
        std::string normal;
        std::string metalRoughness;
        std::string emissive;
        std::string height;
    };

    // The paths of the maps of the color texture at baseColorPath ("Textures/Episode1/Wall_MossyBrick.png"). Needs no files.
    [[nodiscard]] MaterialFilePaths GetMaterialFilePaths(const std::string& baseColorPath);

    // The material of the color texture at baseColorPath: the color and every map whose file exists. A material with a
    // roughness map takes its numbers from it (factors 1); one without gets the default roughness of a material. One with a
    // height map gets the default depth of parallax.
    [[nodiscard]] Material LoadMaterialByFileNames(TextureStore& textures, const std::string& baseColorPath,
                                                   Core::AssetLifetime lifetime);
}
