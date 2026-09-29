#pragma once

#include <cstdint>

namespace Abomination::Core
{
    // A reference to an asset stored in an AssetCache<Asset>: which slot of the cache, and which "generation" of that slot.
    //
    // Why not a pointer: when an asset is removed, its slot is reused for another asset later. A pointer to the old
    // asset would then silently point to the new one (or to freed memory). Removing an asset increases the generation
    // of its slot, so an old handle no longer matches the slot and the cache reports it as invalid instead.
    //
    //   slot 3, generation 1: "Textures/Crate.png"   <- handle {3, 1} is valid
    //   Remove(handle {3, 1})                         -> slot 3 is free, its generation becomes 2
    //   slot 3, generation 2: "Textures/Wall.png"    <- handle {3, 2} is valid, the old {3, 1} is not
    //
    // Asset is only a tag: AssetHandle<GLTexture> and AssetHandle<GLShaderProgram> are different types, so a texture
    // handle cannot be passed where a shader handle is expected. A handle is two numbers: cheap to copy, and it can be
    // compared, stored in components and (together with the asset path) written to a save file.
    template <typename Asset>
    struct AssetHandle
    {
        // Slots start at generation 1, so a default-constructed handle (generation 0) never matches any slot.
        static constexpr std::uint32_t InvalidGeneration = 0;

        std::uint32_t index = 0;
        std::uint32_t generation = InvalidGeneration;

        bool operator==(const AssetHandle&) const = default;
    };
}
