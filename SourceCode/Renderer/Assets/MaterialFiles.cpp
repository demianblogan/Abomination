#include "Renderer/Assets/MaterialFiles.h"

#include <optional>

namespace Abomination::Renderer
{
    MaterialFilePaths GetMaterialFilePaths(const std::string& baseColorPath)
    {
        // "Textures/Episode1/Wall_MossyBrick.png" -> "Textures/Episode1/Wall_MossyBrick" and ".png". The extension is the
        // last dot after the last slash: a dot in the name of a folder is not one.
        const std::size_t lastSlash = baseColorPath.find_last_of('/');
        const std::size_t lastDot = baseColorPath.find_last_of('.');
        const bool hasExtension = lastDot != std::string::npos && (lastSlash == std::string::npos || lastDot > lastSlash);
        const std::string stem = hasExtension ? baseColorPath.substr(0, lastDot) : baseColorPath;
        const std::string extension = hasExtension ? baseColorPath.substr(lastDot) : "";

        return MaterialFilePaths{
            .baseColor = baseColorPath,
            .normal = stem + "_Normal" + extension,
            .metalRoughness = stem + "_MetalRough" + extension,
            .emissive = stem + "_Emissive" + extension,
            .height = stem + "_Height" + extension,
        };
    }

    Material LoadMaterialByFileNames(TextureStore& textures, const std::string& baseColorPath, Core::AssetLifetime lifetime)
    {
        const MaterialFilePaths paths = GetMaterialFilePaths(baseColorPath);

        // The color is the only map every material must have (a missing one shows as the checkerboard). The others hold
        // data, not colors, so they are not converted from sRGB (Raw), and are read smoothly (the color stays crisp, see
        // TextureFiltering); the light given off is a color.
        Material material{.baseColor = textures.Load(paths.baseColor, lifetime, TextureEncoding::SRGB)};
        material.normal = textures.LoadIfExists(paths.normal, lifetime, TextureEncoding::Raw, TextureFiltering::Smooth)
                              .value_or(TextureHandle{});

        if (const std::optional<TextureHandle> map =
                textures.LoadIfExists(paths.metalRoughness, lifetime, TextureEncoding::Raw, TextureFiltering::Smooth);
            map.has_value())
        {
            material.metalRoughness = *map;
            material.roughnessFactor = 1.0f;
            material.metalnessFactor = 1.0f;
        }

        if (const std::optional<TextureHandle> map = textures.LoadIfExists(paths.emissive, lifetime, TextureEncoding::SRGB);
            map.has_value())
        {
            material.emissive = *map;
            material.emissiveFactor = glm::vec3(1.0f);
        }

        if (const std::optional<TextureHandle> map =
                textures.LoadIfExists(paths.height, lifetime, TextureEncoding::Raw, TextureFiltering::Smooth);
            map.has_value())
        {
            material.height = *map;
            material.parallaxDepth = DefaultParallaxDepth;
        }

        return material;
    }
}
