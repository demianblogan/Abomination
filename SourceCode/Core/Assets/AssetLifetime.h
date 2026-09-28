#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <utility>

namespace Abomination::Core
{
    // How long an asset stays loaded. Every asset belongs to one group, and a whole group is removed at once:
    // loading the next level removes everything the previous one brought, without tracking which object used what.
    enum class AssetLifetime : std::uint8_t
    {
        // The whole game: shaders, the weapons and the HUD (later), everything every level needs.
        Global,

        // Until the level is unloaded: its textures and its geometry.
        Level,
    };

    // Names for the debug overlay, in the order of the enum values.
    inline constexpr std::array<std::string_view, 2> AssetLifetimeNames = {"Global", "Level"};

    [[nodiscard]] constexpr std::string_view GetAssetLifetimeName(AssetLifetime lifetime) noexcept
    {
        return AssetLifetimeNames[std::to_underlying(lifetime)];
    }

    // The longer of two lifetimes. An asset asked for by a level and by the whole game must stay for the whole game.
    [[nodiscard]] constexpr AssetLifetime GetLongerLifetime(AssetLifetime first, AssetLifetime second) noexcept
    {
        return first == AssetLifetime::Global || second == AssetLifetime::Global ? AssetLifetime::Global
                                                                                 : AssetLifetime::Level;
    }
}
