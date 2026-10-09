#pragma once

#include "Renderer/Assets/TextureStore.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Abomination::Renderer
{
    // A material by default: what a surface without maps of its own is. Rough, not metal: plaster, stone, wood.
    inline constexpr float DefaultRoughness = 0.8f;

    // How deep the relief of parallax occlusion mapping goes, in texture coordinates (the texture once across is 1): 0.05 is
    // 3 texels of a texture 64 texels wide, about the depth of the mortar between bricks.
    inline constexpr float DefaultParallaxDepth = 0.05f;

    // What a surface is made of (see Documentation/ARCHITECTURE.md, section 6, materials): the maps the lighting shader reads
    // and the numbers they are multiplied by. The convention of glTF (metallic-roughness), so the material of a model file is
    // taken over as it is.
    //
    // A small value of handles and numbers, copied into every component and model part that uses it. A map the material
    // does not have is an invalid handle: the renderer reads a built-in texture of one texel instead (see
    // TextureStore::Get with BuiltInTexture), so a material made with only a color is complete.
    struct Material
    {
        // The color (sRGB) and alpha of the surface, times baseColorFactor. Without it the texture is the checkerboard of a
        // missing file: a surface must have a color.
        TextureHandle baseColor;
        glm::vec4 baseColorFactor{1.0f};

        // Where the surface faces at every texel, in the axes of the texture (see MeshVertex::tangent): x along U, y along
        // V, z out of the surface, each from -1..1 stored as 0..1 (Raw). Without it the surface is flat.
        TextureHandle normal;

        // Green: roughness (0 a mirror .. 1 fully matte), blue: metalness (0 not a metal, 1 a metal), times the factors
        // below (Raw). Without it the factors alone are the roughness and the metalness.
        TextureHandle metalRoughness;
        float roughnessFactor = DefaultRoughness;
        float metalnessFactor = 0.0f;

        // Light the surface gives off itself (sRGB), times emissiveFactor, which may be above 1 (HDR). Without it nothing.
        TextureHandle emissive;
        glm::vec3 emissiveFactor{0.0f};

        // The height of the surface at every texel (white high, black low; Raw), for parallax occlusion mapping: the
        // lighting shader follows the eye into the relief, so stones hide the joints behind them when looked at from the
        // side. parallaxDepth is how deep black lies below white, in texture coordinates; 0 (and no map) is a flat surface.
        TextureHandle height;
        float parallaxDepth = 0.0f;
    };
}
